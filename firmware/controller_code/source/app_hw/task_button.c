// #include "main.h"
// #include "ece453_pins.h"
// #include "task_console.h"
// #include "FreeRTOS_CLI.h"
// #include "task_button.h"

// /* --- GLOBAL SHARED VARIABLE --- */
// volatile bool g_btn_pressed = false;

// /* --- CLI COMMAND --- */
// static BaseType_t cli_handler_btn(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
// static const CLI_Command_Definition_t cmd_btn = {
//     "btn",
//     "\r\nbtn read: Checks P9.5 status\r\n",
//     cli_handler_btn,
//     1
// };

// static BaseType_t cli_handler_btn(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
// {
//     snprintf(pcWriteBuffer, xWriteBufferLen, "Button State: %s\r\n", g_btn_pressed ? "PRESSED" : "RELEASED");
//     return pdFALSE;
// }

// /* --- THE TASK --- */
// void task_button(void *arg)
// {
//     (void)arg;
//     task_print_info("BTN: Task Started");

//     while (1) {
//         /* Read Pin (Active Low: 0=Pressed, 1=Released) */
//         bool pin_state = cyhal_gpio_read(MOD_1_PIN_IO_BUTTON);
        
//         // Invert logic so 'true' means 'pressed'
//         g_btn_pressed = !pin_state;

//         vTaskDelay(pdMS_TO_TICKS(20)); // Run at 50Hz
//     }
// }

// /* --- THE INIT FUNCTION (Matches your project pattern) --- */
// void task_button_init(void)
// {
//     cy_rslt_t rslt;

//     /* 1. Initialize P9.5 */
//     /* Schematic shows external Pull-Up (R3), so we use DRIVE_NONE */
//     rslt = cyhal_gpio_init(MOD_1_PIN_IO_BUTTON, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    
//     if (rslt != CY_RSLT_SUCCESS) {
//         task_print_error("BTN: GPIO Init Failed");
//         return;
//     }

//     /* 2. Register CLI */
//     FreeRTOS_CLIRegisterCommand(&cmd_btn);

//     /* 3. Create the Task */
//     // We handle the creation HERE, so main.c remains clean
//     xTaskCreate(task_button, "BTN", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
// }

#include "main.h"
#include "ece453_pins.h"
#include "task_console.h"
#include "FreeRTOS_CLI.h"
#include "task_button.h"
#include "task_motor.h" /* Required for Haptic Control */
#include "task_imu.h"   /* Required for Calibration */

/* --- GLOBAL SHARED VARIABLE --- */
volatile bool g_btn_pressed = false;

/* --- CLI COMMAND --- */
static BaseType_t cli_handler_btn(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static const CLI_Command_Definition_t cmd_btn = {
    "btn",
    "\r\nbtn read: Checks P9.5 status\r\n",
    cli_handler_btn,
    1
};

static BaseType_t cli_handler_btn(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    snprintf(pcWriteBuffer, xWriteBufferLen, "Button State: %s\r\n", g_btn_pressed ? "PRESSED" : "RELEASED");
    return pdFALSE;
}

/* --- THE TASK --- */
void task_button(void *arg)
{
    (void)arg;
    task_print_info("BTN: Task Started");

    bool last_state = false;
    bool current_state = false;

    while (1) {
        /* Read Pin (Active Low: 0=Pressed, 1=Released) */
        /* We invert it so TRUE = Pressed */
        current_state = !cyhal_gpio_read(MOD_1_PIN_IO_BUTTON);
        
        /* Update global variable */
        g_btn_pressed = current_state;

        /* EDGE DETECTION: Check if button was JUST pressed */
        if (current_state == true && last_state == false)
        {
            task_print_info("BTN: Button Pressed - Calibrating & Rumbling");

            /* 1. Trigger Motor Rumble (Intensity 100%, 300ms) */
            motor_message_t msg;
            msg.command = MOTOR_PULSE_START;
            msg.intensity = 100; 
            msg.duration_ms = 300;
            
            if (q_haptics != NULL) {
                xQueueSendToBack(q_haptics, &msg, 0);
            }

            /* 2. Trigger IMU Calibration */
            task_imu_req_calibration();
        }

        /* Save state for next loop */
        last_state = current_state;

        vTaskDelay(pdMS_TO_TICKS(50)); // Run at 20Hz (Sufficient for button)
    }
}

/* --- THE INIT FUNCTION --- */
void task_button_init(void)
{
    cy_rslt_t rslt;

    /* Initialize P9.5 */
    rslt = cyhal_gpio_init(MOD_1_PIN_IO_BUTTON, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false);
    
    if (rslt != CY_RSLT_SUCCESS) {
        task_print_error("BTN: GPIO Init Failed");
        return;
    }

    /* Register CLI */
    FreeRTOS_CLIRegisterCommand(&cmd_btn);

    /* Create the Task */
    xTaskCreate(task_button, "BTN", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
}