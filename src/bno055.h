#ifndef BNO055_H
#define BNO055_H

#include "cyhal.h"
#include "cybsp.h"

/* BNO055 Register Map (Partial) */
#define BNO055_CHIP_ID_ADDR      0x00
#define BNO055_OPR_MODE_ADDR     0x3D
#define BNO055_SYS_TRIGGER_ADDR  0x3F
#define BNO055_AXIS_MAP_CONFIG   0x41
#define BNO055_AXIS_MAP_SIGN     0x42

/* Data Registers */
#define BNO055_ACCEL_DATA_X_LSB_ADDR 0x08
#define BNO055_EULER_H_LSB_ADDR      0x1A
#define BNO055_QUATERNION_DATA_W_LSB_ADDR 0x20

/* Operation Modes */
#define OPERATION_MODE_CONFIG    0x00
#define OPERATION_MODE_NDOF      0x0C

/* UART Protocol Definitions */
#define BNO_UART_START_BYTE      0xAA
#define BNO_UART_WRITE           0x00
#define BNO_UART_READ            0x01

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_vec3_t;

typedef struct {
    int16_t w;
    int16_t x;
    int16_t y;
    int16_t z;
} bno055_quat_t;

/**
 * @brief Initialize the BNO055 over UART.
 * Configures the sensor to NDOF mode and enables the external crystal.
 * * @param uart_obj Pointer to the initialized PSoC UART object
 * @return cy_rslt_t CY_RSLT_SUCCESS if initialized, error otherwise
 */
cy_rslt_t bno055_init(cyhal_uart_t *uart_obj);

/**
 * @brief Read Euler angles (Heading, Roll, Pitch)
 * @param euler Pointer to struct to store data (16 LSB = 1 Degree)
 */
cy_rslt_t bno055_read_euler(bno055_vec3_t *euler);

#endif