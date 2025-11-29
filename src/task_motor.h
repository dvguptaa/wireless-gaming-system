#ifndef __TASK_MOTOR_H__
#define __TASK_MOTOR_H__

#include <stdint.h>
#include "main.h"
#include "ece453_pins.h"

// /* Forward declaration for FreeRTOS queue handle */
// struct QueueDefinition;
// typedef struct QueueDefinition * QueueHandle_t;

/* Pin Definition */
#define PIN_HAPTIC_PWM MOD_1_PIN_PWM

/* Command Types */
typedef enum
{
    MOTOR_PULSE_START = 0
} motor_command_t;

/* Haptic Message Struct */
typedef struct
{
    motor_command_t command;
    union
    {
        uint8_t intensity;      /* 0-100% vibration strength */
        uint16_t duration_ms;   /* Length of the pulse in milliseconds */
    };
} motor_message_t;

/* Task Resources */
extern QueueHandle_t q_haptics;

/* Function Prototype */
void task_motor_init(void);

#endif /* __TASK_MOTOR_H__ */

