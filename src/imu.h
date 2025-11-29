/**
 * @file imu.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __IMU_H__
#define __IMU_H__

#include <stdint.h>
#include <stdbool.h>

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cyhal_hw_types.h"
#include "spi.h"

#include <stdint.h>

// LSM6DSMTR register addresses
#define IMU_REG_CTRL1_XL     0x10  // Accelerometer control
#define IMU_REG_CTRL2_G      0x11  // Gyroscope control
#define IMU_REG_OUTX_L_G     0x22  // Gyro output start
#define IMU_REG_OUTX_L_XL    0x28  // Accel output start
#define IMU_REG_WHO_AM_I     0x0F  // Device ID
#define IMU_REG_CTRL3_C      0x12  // Control register 3

// Configuration values
#define ODR_104HZ    0x40  // Output data rate = 104 Hz
#define FS_XL_2G     0x00  // ±2g
#define FS_G_250DPS  0x00  // ±250 dps

// Conversion factors
#define ACCEL_SENS_2G   (2.0f / 32768.0f)   // g/LSB
#define GYRO_SENS_250DPS (250.0f / 32768.0f) // dps/LSB

bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin);
void imu_write_reg(uint8_t reg, uint8_t value);
uint8_t imu_read_reg(uint8_t reg);
void imu_read_registers(uint8_t reg, uint8_t *buffer, uint8_t length);
void imu_read_accel_gyro(float *ax, float *ay, float *az, float *gx, float *gy, float *gz);

#endif