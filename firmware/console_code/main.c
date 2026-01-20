// /**
//  * @file main.c
//
//  * @brief 
//  * @version 0.1

// #include "main.h"
// #include "cy_retarget_io.h" /* Added for printf support */

// #include "IR.h"
// #include "source/app_hw/task_console.h"
// #include "source/app_hw/task_blink.h"
// #include "source/app_hw/task_motor.h"
// #include "source/app_hw/task_ble.h" 
// #include "RPI.h"
// #include "console.h"
// #include "i2c.h"


// int main(void)
// {
//     cy_rslt_t rslt;

//     /* Initialize the device and board peripherals */
//     rslt = cybsp_init() ;
//     CY_ASSERT(rslt == CY_RSLT_SUCCESS);

//     __enable_irq();

//     /* Initialize retarget-io to use the debug UART port for printf */
//     cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

//     /* Initialize Tasks */
//     task_console_init();
//     task_ble_init(); 

    
//     //main_task();
//     sensor_task();
//     console_init();
//     sensor_test();
    
    
//     //sensor_test();
//     //uart_test();

//     /* Start the FreeRTOS scheduler */
//     vTaskStartScheduler();
    
//     for (;;)
//     {
    
//     }
// }

// /* [] END OF FILE */

/**
 * @file main.c
 * @brief Main entry point - BLE Debugging Mode
 */
#include "main.h"
#include "cy_retarget_io.h"
#include "source/app_hw/task_console.h"
#include "source/app_hw/task_ble.h" 
#include "console.h"
#include "i2c.h"

int main(void)
{
    cy_rslt_t rslt;

    /* Initialize the device and board peripherals */
    rslt = cybsp_init() ;
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    __enable_irq();

    /* 1. Initialize UART with explicit 115200 baud rate */
    // cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, 115200);
    // printf("[MAIN] Debug UART TX=0x%X, RX=0x%X\r\n", CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX);

    /* 2. Print a boot message immxediately to verify connection */
    /* If you don't see this, check your Terminal Baud Rate (115200) or Com Port */
    // printf("\r\n\r\n");
    // printf("==========================================\r\n");
    // printf("[SYSTEM] Booting...\r\n");
    // printf("[SYSTEM] UART Initialized at 115200 baud\r\n");
    // printf("==========================================\r\n");


    /* Commenting out other tasks to prevent blocking/crashes during BLE debug */
    // task_console_init(); // --Causing blocking issue for some reason
    task_ble_init();
    console_init();

    
    // printf("[SYSTEM] Starting FreeRTOS Scheduler...\r\n");
    Cy_SysLib_Delay(1000);
     vTaskStartScheduler();
    
    /* Should never get here */
    for (;;)
    {
    }
}
/* [] END OF FILE */