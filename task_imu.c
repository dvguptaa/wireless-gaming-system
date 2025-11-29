#include "task_imu.h"
#include "cyhal_hw_types.h"
#include "ece453_pins.h"
#include "main.h"
#include "task_console.h"
#include "FreeRTOS_CLI.h"
#include "bno055.h"
#include <string.h>
#include <stdio.h>

/* UART Object for BNO055 */
static cyhal_uart_t bno_uart_obj;
static bool imu_initialized = false;

/* Forward declaration */
static BaseType_t cli_handler_imu(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/* CLI command */
static const CLI_Command_Definition_t cmd_imu = {
    "imu",
    "\r\nimu read\r\n",
    cli_handler_imu,
    1
};

/* CLI handler - Reads Euler Angles from BNO055 */
static BaseType_t cli_handler_imu(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *param;
    BaseType_t param_len;
    bno055_vec3_t euler = {0, 0, 0};
    cy_rslt_t result;

    memset(pcWriteBuffer, 0, xWriteBufferLen);

    param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
    if (param == NULL || param_len != 4 || strncmp(param, "read", 4) != 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: imu read\r\n");
        return pdFALSE;
    }

    if (!imu_initialized) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "IMU not initialized\r\n");
        return pdFALSE;
    }

    result = bno055_read_euler(&euler);

    if (result == CY_RSLT_SUCCESS) {
        float heading = (float)euler.x / 16.0f;
        float roll    = (float)euler.y / 16.0f;
        float pitch   = (float)euler.z / 16.0f;

        snprintf(pcWriteBuffer, xWriteBufferLen,
                 "Heading: %.2f | Roll: %.2f | Pitch: %.2f\r\n",
                 heading, roll, pitch);
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to read BNO055\r\n");
    }

    return pdFALSE;
}

bool task_imu_resources_init(void)
{
    cy_rslt_t rslt;

    /* Reset pin is already initialized in main.c, so skip it here */

    /* Initialize UART */
    const cyhal_uart_cfg_t uart_config = {
        .data_bits = 8,
        .stop_bits = 1,
        .parity = CYHAL_UART_PARITY_NONE,
        .rx_buffer = NULL,
        .rx_buffer_size = 0
    };

    rslt = cyhal_uart_init(&bno_uart_obj, 
                           MOD_1_PIN_UART_IMU_TX, 
                           MOD_1_PIN_UART_IMU_RX, 
                           NC, NC, NULL, 
                           &uart_config);

    if (rslt != CY_RSLT_SUCCESS) {
        task_print_info("IMU: UART init failed");
        return false;
    }

    uint32_t actual_baud;
    cyhal_uart_set_baud(&bno_uart_obj, 115200, &actual_baud);

    FreeRTOS_CLIRegisterCommand(&cmd_imu);

    if (xTaskCreate(task_imu, "IMU", 10 * configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        task_print_info("IMU: Failed to create task");
        return false;
    }

    return true;
}

void task_imu(void *arg)
{
    (void)arg;
    cy_rslt_t result;

    task_print_info("IMU: Task Started");
    
    uint8_t retry_count = 0;
    const uint8_t MAX_RETRIES = 5;

    while (retry_count < MAX_RETRIES) {
        
        if (retry_count > 0) {
             task_print_warning("IMU: Retrying init (Attempt %d)...", retry_count + 1);
        }

        /* * FIX: PERFORM HARDWARE RESET HERE
         * This ensures that if the sensor is stuck, we unstick it.
         */
        task_print_info("IMU: Performing Hardware Reset...");
        cyhal_gpio_write(MOD_1_PIN_IO_IMU_nRESET, false); // Hold Low
        vTaskDelay(pdMS_TO_TICKS(10));
        cyhal_gpio_write(MOD_1_PIN_IO_IMU_nRESET, true);  // Release High

        /* * FIX: WAIT FOR BOOT
         * Datasheet says ~650ms. We wait 800ms to be safe.
         * The previous 100ms was too short for a Soft Reset.
         */
        vTaskDelay(pdMS_TO_TICKS(800));

        /* Drain UART and attempt init */
        cyhal_uart_clear(&bno_uart_obj);
        
        result = bno055_init(&bno_uart_obj);

        if (result == CY_RSLT_SUCCESS) {
            imu_initialized = true;
            task_print_info("BNO055: Initialized Successfully!");
            break; 
        } else {
            retry_count++;
            // Small delay before next retry loop (though the reset delay covers us)
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
    }

    if (!imu_initialized) {
        task_print_error("CRITICAL: BNO055 Failed to Initialize.");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}