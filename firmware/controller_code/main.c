/**
 * @file main.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.2
 * @date 2024-05-14
 * * @copyright Copyright (c) 2024
 * */
#include "main.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_blink.h"
#include "spi.h"
#include "task_eeprom.h"
#include "task_imu.h"
#include "task_motor.h"
#include "task_bluetooth.h"
#include "ece453_pins.h"
#include "task_button.h"

/* External variables from spi.c */
extern cyhal_spi_t mSPI;
extern SemaphoreHandle_t Semaphore_SPI;

int main(void)
{
    cy_rslt_t rslt;

    /* Initialize the device and board peripherals */
    rslt = cybsp_init();
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    __enable_irq();

    /* * FIX: Initialize IMU Reset Pin as HIGH (Idle).
     * We do NOT toggle it here. We let task_imu handle the reset sequence
     * so it can control the specific timing and retries.
     */
    rslt = cyhal_gpio_init(MOD_1_PIN_IO_IMU_nRESET, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    /* Initialize SPI bus */
    rslt = spi_init(MODULE_SITE_1);  
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    /* Initialize EEPROM Chip Select (CS) pin as GPIO output */
    rslt = cyhal_gpio_init(MOD_1_PIN_SPI_CS_N, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS); 
    
    /* Initialize console task */
    task_console_init();

    /* Initialize blink task */
    task_blink_init();

    /* Initialize EEPROM resources */
    task_eeprom_resources_init(&Semaphore_SPI, &mSPI, MOD_1_PIN_SPI_CS_N);
    
    /* Initialize IMU resources (Now UART based) */
    if (!task_imu_resources_init()) {
        task_print_error("IMU initialization failed!");
    }

    /* Initialize Button Task */
    task_button_init();
    
    /* Initialize motor task */
    task_motor_init();

    /* --- ADD THIS --- */
    /* Initialize Bluetooth Task */
    task_bluetooth_init();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    for (;;)
    {
    }
}
/* [] END OF FILE */