#include "task_imu.h"
#include "task_eeprom.h"
#include "cyhal_hw_types.h"
#include "ece453_pins.h"
#include "main.h"
#include "task_console.h"
#include "FreeRTOS_CLI.h"
#include "bno055.h"
#include "semphr.h"
#include <string.h>
#include <stdlib.h> // for abs()

/* --- TUNING CONFIGURATION --- */
/* Trigger: Tilt > 20 degrees to start (Prevent accidental triggers) */
#define TRIGGER_ANGLE_DEG       20.0f
/* Release: Return < 10 degrees to stop (Hysteresis) */
#define RELEASE_ANGLE_DEG       10.0f

#define THRESH_TRIGGER          (int16_t)(TRIGGER_ANGLE_DEG * 16)
#define THRESH_RELEASE          (int16_t)(RELEASE_ANGLE_DEG * 16)

#define EEPROM_CALIB_ADDR       0x0000 
#define CALIB_MAGIC_NUM         0xAB
#define MOVING_AVG_SIZE         10      /* Smoothing filter size */

/* Global State */
static cyhal_uart_t bno_uart_obj;
static bool imu_initialized = false;
static imu_data_t latest_imu_data = {0, 0, 0, GESTURE_NONE};
static imu_calib_t current_calib = {0, 0, 0}; 
static SemaphoreHandle_t imu_data_mutex = NULL;

/* Logic State */
static imu_gesture_t locked_gesture = GESTURE_NONE;

/* Filter State */
static int16_t roll_history[MOVING_AVG_SIZE] = {0};
static int16_t pitch_history[MOVING_AVG_SIZE] = {0};
static uint8_t history_idx = 0;
static bool history_filled = false;

static volatile bool calib_req_flag = false;

void task_imu_req_calibration(void) { calib_req_flag = true; }

static BaseType_t cli_handler_imu(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t cmd_imu = {"imu", "\r\nimu <read|calibrate>\r\n", cli_handler_imu, -1};

/* Helper: Update Moving Average */
static void update_filter(int16_t new_roll, int16_t new_pitch, int16_t *avg_roll, int16_t *avg_pitch)
{
    if (!history_filled) {
        for(int i=0; i<MOVING_AVG_SIZE; i++) {
            roll_history[i] = new_roll;
            pitch_history[i] = new_pitch;
        }
        history_filled = true;
    } else {
        roll_history[history_idx] = new_roll;
        pitch_history[history_idx] = new_pitch;
        history_idx = (history_idx + 1) % MOVING_AVG_SIZE;
    }

    int32_t r_sum = 0;
    int32_t p_sum = 0;
    for(int i=0; i<MOVING_AVG_SIZE; i++) {
        r_sum += roll_history[i];
        p_sum += pitch_history[i];
    }
    *avg_roll = (int16_t)(r_sum / MOVING_AVG_SIZE);
    *avg_pitch = (int16_t)(p_sum / MOVING_AVG_SIZE);
}

/* Helper: Determine Gesture with Hysteresis */
static imu_gesture_t detect_gesture(int16_t curr_roll, int16_t curr_pitch)
{
    int16_t delta_r = curr_roll - current_calib.center_roll;
    int16_t delta_p = curr_pitch - current_calib.center_pitch;
    int16_t abs_r = abs(delta_r);
    int16_t abs_p = abs(delta_p);

    if (locked_gesture == GESTURE_NONE)
    {
        /* IDLE: Wait for Trigger Threshold */
        bool p_trigger = (abs_p > THRESH_TRIGGER);
        bool r_trigger = (abs_r > THRESH_TRIGGER);

        if (p_trigger || r_trigger)
        {
            /* Determine Dominant Axis */
            if (abs_p > abs_r) {
                if (delta_p > 0) locked_gesture = GESTURE_DOWN;
                else locked_gesture = GESTURE_UP;
            } else {
                if (delta_r > 0) locked_gesture = GESTURE_RIGHT;
                else locked_gesture = GESTURE_LEFT;
            }
        }
    }
    else
    {
        /* LOCKED: Wait for Release Threshold */
        bool release = false;
        switch(locked_gesture) {
            case GESTURE_UP:
            case GESTURE_DOWN:  if (abs_p < THRESH_RELEASE) release = true; break;
            case GESTURE_LEFT:
            case GESTURE_RIGHT: if (abs_r < THRESH_RELEASE) release = true; break;
            default: release = true; break;
        }
        if (release) locked_gesture = GESTURE_NONE;
    }
    return locked_gesture;
}

/* Helper: Save Calibration */
static void save_calibration(int16_t roll, int16_t pitch)
{
    current_calib.center_roll = roll;
    current_calib.center_pitch = pitch;
    current_calib.magic_num = CALIB_MAGIC_NUM;
    
    // Optional: Send to EEPROM Task here
    task_print_info("IMU: Calibration Set");
}

/* Thread Safe Getter */
void task_imu_get_data(imu_data_t *data)
{
    if (imu_data_mutex && xSemaphoreTake(imu_data_mutex, pdMS_TO_TICKS(10)) == pdPASS) {
        *data = latest_imu_data;
        xSemaphoreGive(imu_data_mutex);
    }
}

/* CLI */
static BaseType_t cli_handler_imu(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    snprintf(pcWriteBuffer, xWriteBufferLen, "IMU Active\r\n");
    return pdFALSE;
}

/* MAIN TASK */
void task_imu(void *arg)
{
    (void)arg;
    cy_rslt_t result;
    bno055_vec3_t euler_read;
    int16_t avg_roll = 0, avg_pitch = 0;

    imu_data_mutex = xSemaphoreCreateMutex();
    
    /* Hardware Reset & Init */
    cyhal_gpio_write(MOD_1_PIN_IO_IMU_nRESET, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    cyhal_gpio_write(MOD_1_PIN_IO_IMU_nRESET, true);
    vTaskDelay(pdMS_TO_TICKS(800));
    
    const cyhal_uart_cfg_t uart_config = {
        .data_bits = 8, .stop_bits = 1, .parity = CYHAL_UART_PARITY_NONE, .rx_buffer = NULL, .rx_buffer_size = 0
    };
    cyhal_uart_init(&bno_uart_obj, MOD_1_PIN_UART_IMU_TX, MOD_1_PIN_UART_IMU_RX, NC, NC, NULL, &uart_config);
    uint32_t baud;
    cyhal_uart_set_baud(&bno_uart_obj, 115200, &baud);
    
    if (bno055_init(&bno_uart_obj) == CY_RSLT_SUCCESS) {
        imu_initialized = true;
        task_print_info("BNO055: Initialized (Tilt Mode)");
    } else {
        task_print_error("BNO055: Init Failed");
    }

    while (1) {
        if (imu_initialized) {
            result = bno055_read_euler(&euler_read);

            if (calib_req_flag) {
                calib_req_flag = false;
                save_calibration(avg_roll, avg_pitch);
            }
            
            if (result == CY_RSLT_SUCCESS) {
                update_filter(euler_read.y, euler_read.z, &avg_roll, &avg_pitch);

                if (xSemaphoreTake(imu_data_mutex, pdMS_TO_TICKS(10)) == pdPASS) {
                    latest_imu_data.heading = euler_read.x;
                    latest_imu_data.roll    = avg_roll;
                    latest_imu_data.pitch   = avg_pitch;
                    latest_imu_data.gesture = detect_gesture(avg_roll, avg_pitch);
                    xSemaphoreGive(imu_data_mutex);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
    }
}

bool task_imu_resources_init(void) {
    FreeRTOS_CLIRegisterCommand(&cmd_imu);
    return (xTaskCreate(task_imu, "IMU", 10*configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) == pdPASS);
}