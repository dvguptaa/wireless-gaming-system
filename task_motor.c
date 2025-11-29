#include "main.h"
#include "task_console.h"
#include "FreeRTOS_CLI.h"
#include "task_motor.h"
#include "cyhal.h"
#include "cyhal_pwm.h"
#include "cyhal_gpio.h"


/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_motor(void *param);
static void motor_set_pwm(uint8_t intensity);
static BaseType_t cli_handler_haptic(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
/* Queue used to send commands to control the haptic motor */
QueueHandle_t q_haptics;

/* PWM object for haptic motor control */
cyhal_pwm_t pwm_motor_obj;

/* The CLI command definition for the pulse command */
static const CLI_Command_Definition_t xPulse =
    {
        "pulse",                                    /* command text */
        "\r\npulse <intensity> <duration_ms>\r\n",  /* command help text */
        cli_handler_haptic,                        /* The function to run. */
        2                                           /* The user can enter 2 parameters */
    };

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Set the PWM duty cycle for the haptic motor
 * @param intensity PWM intensity (0-100), will be clamped to 100
 */
static void motor_set_pwm(uint8_t intensity)
{
    /* Clamp intensity to maximum of 100 */
    if (intensity > 100)
    {
        intensity = 100;
    }

    /* Set the PWM duty cycle as a percentage */
    cyhal_pwm_set_duty_cycle(&pwm_motor_obj, intensity, 100);
}

/**
 * @brief FreeRTOS task that controls the haptic motor
 * @param param Task parameter (unused)
 */
static void task_motor(void *param)
{
    motor_message_t message;

    /* Suppress warning for unused parameter */
    (void)param;

    /* Repeatedly running part of the task */
    for (;;)
    {
        /* Block indefinitely waiting for a message from the queue */
        if (xQueueReceive(q_haptics, &message, portMAX_DELAY) == pdPASS)
        {
            /* Start the pulse: set PWM with the message's intensity */
            motor_set_pwm(message.intensity);

            /* Wait for the specified duration */
            vTaskDelay(pdMS_TO_TICKS(message.duration_ms));

            /* Turn the motor off */
            motor_set_pwm(0);

            /* Print info message after pulse is executed */
            task_print_info("Haptic pulse completed: intensity=%d%%, duration=%d ms",
                           message.intensity, message.duration_ms);
        }
    }
}

/**
 * @brief FreeRTOS CLI Handler for the 'pulse' command
 * @param pcWriteBuffer Buffer to write command output
 * @param xWriteBufferLen Length of the write buffer
 * @param pcCommandString The command string entered by the user
 * @return pdFALSE when command processing is complete
 */
static BaseType_t cli_handler_haptic(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *param;
    BaseType_t param_len;
    char *end_ptr;
    char param_buffer[16];
    long intensity_val;
    long duration_val;
    motor_message_t message;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL. */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    /* Clear output buffer */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    /* Get first parameter (intensity) */
    param = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the 1st parameter. */
        &param_len             /* Store the parameter string length. */
    );

    if (param == NULL || param_len <= 0)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Missing intensity parameter\r\n");
        return pdFALSE;
    }

    /* Safely copy parameter to local buffer for strtol */
    if (param_len >= sizeof(param_buffer))
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Intensity parameter too long\r\n");
        return pdFALSE;
    }
    memset(param_buffer, 0, sizeof(param_buffer));
    strncpy(param_buffer, param, param_len);

    /* Convert intensity string to number (base 10) */
    intensity_val = strtol(param_buffer, &end_ptr, 10);
    if (*end_ptr != '\0' || intensity_val < 0 || intensity_val > 100)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid intensity (must be 0-100)\r\n");
        return pdFALSE;
    }

    /* Get second parameter (duration_ms) */
    param = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        2,                      /* Return the 2nd parameter. */
        &param_len             /* Store the parameter string length. */
    );

    if (param == NULL || param_len <= 0)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Missing duration_ms parameter\r\n");
        return pdFALSE;
    }

    /* Safely copy parameter to local buffer for strtol */
    if (param_len >= sizeof(param_buffer))
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Duration parameter too long\r\n");
        return pdFALSE;
    }
    memset(param_buffer, 0, sizeof(param_buffer));
    strncpy(param_buffer, param, param_len);

    /* Convert duration string to number (base 10) */
    duration_val = strtol(param_buffer, &end_ptr, 10);
    if (*end_ptr != '\0' || duration_val <= 0)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid duration (must be > 0)\r\n");
        return pdFALSE;
    }

    /* Pack the validated values into the message struct */
    message.command = MOTOR_PULSE_START;
    message.intensity = (uint8_t)intensity_val;
    message.duration_ms = (uint16_t)duration_val;

    /* Send the message to the haptics queue */
    if (xQueueSendToBack(q_haptics, &message, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to send pulse command to queue\r\n");
        return pdFALSE;
    }

    /* Provide confirmation message */
    snprintf(pcWriteBuffer, xWriteBufferLen, "Pulse command sent: intensity=%d%%, duration=%ld ms\r\n",
             (uint8_t)intensity_val, duration_val);

    /* Indicate that the command has completed */
    return pdFALSE;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Initialize the haptic motor task, PWM, queue, and CLI command
 */
void task_motor_init(void)
{
    cy_rslt_t rslt;

    /* Initialize the PWM object on PIN_HAPTIC_PWM */
    /* Frequency: 10000 Hz (10 kHz), Start with 0% duty cycle */
    rslt = cyhal_pwm_init(&pwm_motor_obj, PIN_HAPTIC_PWM, NULL);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    /* Set the PWM period for 10 kHz (period = 1/frequency = 100 microseconds) */
    rslt = cyhal_pwm_set_period(&pwm_motor_obj, 100, 0);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    /* Start PWM with 0% duty cycle */
    rslt = cyhal_pwm_start(&pwm_motor_obj);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
    cyhal_pwm_set_duty_cycle(&pwm_motor_obj, 0, 100);

    /* Create the queue used to control the haptic motor */
    q_haptics = xQueueCreate(5, sizeof(motor_message_t));
    CY_ASSERT(q_haptics != NULL);

    /* Register the CLI command */
    FreeRTOS_CLIRegisterCommand(&xPulse);

    /* Create the task that will control the haptic motor */
    xTaskCreate(
        task_motor,
        "Task_Motor",
        5 * configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL);
}

