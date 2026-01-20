/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


#include "platform.h"
#include "IR.h"
#include "i2c.h"
#include "cy_syslib.h"
#include "cyhal_system.h"
#include "FreeRTOS.h"


uint8_t VL53L4CD_RdDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t *value)
{
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	uint8_t read_buffer[4];

	// Register address is 16-bit, send MSB first (big-endian)
	write_buffer[0] = (RegisterAdress >> 8) & 0xFF;  // MSB
	write_buffer[1] = RegisterAdress & 0xFF;         // LSB

	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	
	// First write the register address
	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_buffer, 2, 0, false);
	
	if (rslt == CY_RSLT_SUCCESS)
	{
		// Then read 4 data bytes
		rslt = cyhal_i2c_master_read(&i2c_master_obj1, dev, read_buffer, 4, 0, true);
		
		if (rslt == CY_RSLT_SUCCESS)
		{
			// Reconstruct 32-bit value from big-endian bytes
			*value = ((uint32_t)read_buffer[0] << 24) | 
			         ((uint32_t)read_buffer[1] << 16) |
			         ((uint32_t)read_buffer[2] << 8)  |
			         ((uint32_t)read_buffer[3]);
		}
	}
	
	xSemaphoreGive(Semaphore_I2C);

	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;
}

uint8_t VL53L4CD_RdWord(Dev_t dev, uint16_t RegisterAdress, uint16_t *value)
{
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	uint8_t read_buffer[2];

	// Register address is 16-bit, send MSB first (big-endian)
	write_buffer[0] = (RegisterAdress >> 8) & 0xFF;  // MSB
	write_buffer[1] = RegisterAdress & 0xFF;         // LSB

	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	
	// First write the register address
	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_buffer, 2, 0, false);
	
	if (rslt == CY_RSLT_SUCCESS)
	{
		// Then read 2 data bytes
		rslt = cyhal_i2c_master_read(&i2c_master_obj1, dev, read_buffer, 2, 0, true);
		                                                                                                                                                                                                                                                                                                                                              
		if (rslt == CY_RSLT_SUCCESS)
		{
			// Reconstruct 16-bit value from big-endian bytes
			*value = (read_buffer[0] << 8) | read_buffer[1];
		}
	}
	
	xSemaphoreGive(Semaphore_I2C);

	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;
}

uint8_t VL53L4CD_RdByte(Dev_t dev, uint16_t RegisterAdress, uint8_t *value)
{

	cy_rslt_t rslt;
	uint8_t write_byte[2];


	write_byte[0] = (RegisterAdress >> 8) & 0xFF; 
	write_byte[1] = RegisterAdress & 0xFF;        

	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	
	// First write the register address
	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_byte, 2, 0, false);
	
	if (rslt == CY_RSLT_SUCCESS)
	{
		// Then read the data byte
		rslt = cyhal_i2c_master_read(&i2c_master_obj1, dev, value, 1, 0, true);
	}
	
	xSemaphoreGive(Semaphore_I2C);
	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;

}

uint8_t VL53L4CD_WrByte(Dev_t dev, uint16_t RegisterAdress, uint8_t value)
{
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	cy_rslt_t rslt;
	uint8_t write_buffer[3];

	write_buffer[0] = (RegisterAdress) >> 8 & 0xFF;
	write_buffer[1] = (RegisterAdress) & 0xFF;
	write_buffer[2] = value;

	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_buffer, 3, 0, true);
	xSemaphoreGive(Semaphore_I2C);

	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;
}

uint8_t VL53L4CD_WrWord(Dev_t dev, uint16_t RegisterAdress, uint16_t value)
{
	cy_rslt_t rslt;
	uint8_t write_buffer[4];
	
	// Register address (big-endian)
	write_buffer[0] = (RegisterAdress >> 8) & 0xFF;  // Address MSB
	write_buffer[1] = RegisterAdress & 0xFF;         // Address LSB
	
	// 16-bit value (big-endian)
	write_buffer[2] = (value >> 8) & 0xFF;           // Value MSB
	write_buffer[3] = value & 0xFF;                  // Value LSB
	
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	
	// Write all 4 bytes in one transaction
	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_buffer, 4, 0, true);
	
	xSemaphoreGive(Semaphore_I2C);
	
	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;
}

uint8_t VL53L4CD_WrDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t value)
{
	cy_rslt_t rslt;
	uint8_t write_buffer[6];
	
	// Register address (big-endian)
	write_buffer[0] = (RegisterAdress >> 8) & 0xFF;  // Address MSB
	write_buffer[1] = RegisterAdress & 0xFF;         // Address LSB
	
	// 32-bit value (big-endian)
	write_buffer[2] = (value >> 24) & 0xFF;          // Value MSB
	write_buffer[3] = (value >> 16) & 0xFF;
	write_buffer[4] = (value >> 8) & 0xFF;
	write_buffer[5] = value & 0xFF;                  // Value LSB
	
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	
	// Write all 6 bytes in one transaction
	rslt = cyhal_i2c_master_write(&i2c_master_obj1, dev, write_buffer, 6, 0, true);
	
	xSemaphoreGive(Semaphore_I2C);
	
	return (rslt == CY_RSLT_SUCCESS) ? 0 : 1;
}

uint8_t VL53L4CD_WaitMs(Dev_t dev, uint32_t TimeMs)
{
	uint8_t status = 0;
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
		vTaskDelay(pdMS_TO_TICKS(TimeMs));
	}
	else
    {
        // Scheduler not started - use blocking delay
        Cy_SysLib_Delay(TimeMs);  // Or cyhal_system_delay_ms(TimeMs);
    }
	
	return status;
}


