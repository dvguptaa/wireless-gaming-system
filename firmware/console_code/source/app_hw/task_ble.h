// #ifndef TASK_BLE_H
// #define TASK_BLE_H

// #include <stdint.h>
// #include <stdbool.h>

// /**
//  * @brief Initializes the BLE Client Task.
//  * This function creates the FreeRTOS task that handles BLE stack initialization,
//  * scanning, connection, and data reception.
//  */
// void task_ble_init(void);

// /**
//  * @brief Sends a command to the Motor/Haptic characteristic on the server.
//  * * @param command 0x00 for OFF, 0x01 for ON (or other value depending on controller logic)
//  */
// void task_ble_send_motor_cmd(uint8_t command);

// #endif /* TASK_BLE_H */

#ifndef TASK_BLE_H
#define TASK_BLE_H

#include <stdint.h>
#include <stdbool.h>



/**
 * @brief Initializes the BLE Client Task.
 */
void task_ble_init(void);

/**
 * @brief Sends a command to the Motor/Haptic characteristic.
 */
void task_ble_send_motor_cmd(uint8_t command);

/* * EXTERN DECLARATION
 * This allows other files (like console.c) to modify the motor request.
 */
extern volatile uint8_t ble_motor_request;

#endif /* TASK_BLE_H */