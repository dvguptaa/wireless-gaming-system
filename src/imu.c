/**
 * @file imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "imu.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"

static cyhal_spi_t *IMU_spi_obj;
static cyhal_gpio_t PIN_IMU_CS_N;

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @param value 
 */
void imu_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2] = {reg & 0x7F, value};
    uint8_t rx_data[2] = {0};

    cyhal_gpio_write(PIN_IMU_CS_N, false);
    cyhal_spi_transfer(IMU_spi_obj, tx_data, sizeof(tx_data), rx_data, sizeof(rx_data), 100);
    cyhal_gpio_write(PIN_IMU_CS_N, true);
}

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @return uint8_t 
 */
uint8_t imu_read_reg(uint8_t reg)
{
    uint8_t tx_data[2] = {reg | 0x80, 0x00};
    uint8_t rx_data[2] = {0};

    cyhal_gpio_write(PIN_IMU_CS_N, false);
    cyhal_spi_transfer(IMU_spi_obj, tx_data, sizeof(tx_data), rx_data, sizeof(rx_data), 100);
    cyhal_gpio_write(PIN_IMU_CS_N, true);

    return rx_data[1];
}

/**
 * @brief Read multiple registers from the IMU
 * 
 * @param reg 
 * @param buffer 
 * @param length 
 */
void imu_read_registers(uint8_t reg, uint8_t *buffer, uint8_t length)
{
    if (length == 0) {
        return;
    }

    // Implement SPI read operation here
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = imu_read_reg(reg + i);
    }
}

/**
 * @brief Read accelerometer and gyroscope data from the IMU
 * 
 * @param ax Pointer to store X-axis acceleration (g)
 * @param ay Pointer to store Y-axis acceleration (g)
 * @param az Pointer to store Z-axis acceleration (g)
 * @param gx Pointer to store X-axis angular velocity (dps)
 * @param gy Pointer to store Y-axis angular velocity (dps)
 * @param gz Pointer to store Z-axis angular velocity (dps)
 */
void imu_read_accel_gyro(float *ax, float *ay, float *az, float *gx, float *gy, float *gz)
{
    uint8_t buffer[12];
    imu_read_registers(IMU_REG_OUTX_L_G, buffer, 12);

    /* Raw Gyroscope Data */
    int16_t raw_gx = (int16_t)(buffer[1] << 8 | buffer[0]);
    int16_t raw_gy = (int16_t)(buffer[3] << 8 | buffer[2]);
    int16_t raw_gz = (int16_t)(buffer[5] << 8 | buffer[4]);

    /* Raw Accelerometer Data */
    int16_t raw_ax = (int16_t)(buffer[7] << 8 | buffer[6]);
    int16_t raw_ay = (int16_t)(buffer[9] << 8 | buffer[8]);
    int16_t raw_az = (int16_t)(buffer[11] << 8 | buffer[10]);

    /* Convert to physical units */
    *gx = raw_gx * GYRO_SENS_250DPS;
    *gy = raw_gy * GYRO_SENS_250DPS;
    *gz = raw_gz * GYRO_SENS_250DPS;
    *ax = raw_ax * ACCEL_SENS_2G;
    *ay = raw_ay * ACCEL_SENS_2G;
    *az = raw_az * ACCEL_SENS_2G;
}

/**
 * @brief Initialize the IMU sensor.  Assumes the that SPI interface has already been initialized.
 *
 * @return true if initialization is successful, false otherwise.
 */
bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    if(spi_obj == NULL)
    {
        return false;
    }

    /* Set the IMU SPI and CS pin */
    IMU_spi_obj = spi_obj;
    PIN_IMU_CS_N = cs_pin;

    // Wait for 15mS after power-up
    cyhal_system_delay_ms(15);

    // Verify the WHO_AM_I register
    uint8_t who_am_i = imu_read_reg(IMU_REG_WHO_AM_I);
    if (who_am_i != 0x6A)
    {
        return false;
    }

    // Reset the IMU
    imu_write_reg(IMU_REG_CTRL3_C, 0x01); // CTRL3_C register

    // Wait for the reset to complete
    do
    {
        cyhal_system_delay_ms(1);
    } while (imu_read_reg(IMU_REG_CTRL3_C) & 0x01);

    // Configure the IMU:  104 Hz, ±2g, ±250 dps
    imu_write_reg(IMU_REG_CTRL1_XL, ODR_104HZ | FS_XL_2G);
    imu_write_reg(IMU_REG_CTRL2_G, ODR_104HZ | FS_G_250DPS);

    return true;
}