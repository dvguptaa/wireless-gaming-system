#include "IR.h"
#include "cy_result.h"
#include "i2c.h"
#include "VL53L4CD_api.h"
#include "VL53L4CD_calibration.h"
#include "cyhal_uart.h"
#include "uart.h"
#include <inttypes.h>
#include <stdint.h>

Dev_t dev, new_dev;
uint8_t status;
uint8_t isReady = 0xFF;
uint16_t sensor_id;
VL53L4CD_ResultsData_t results;

static bool sensor_initialized = false;
static bool ranging_started = false;



void sensor_poll(void) 
{
    if (!sensor_initialized) {
        return;
    }
    
    // Start ranging if not already started
    if (!ranging_started) {
        status = VL53L4CD_StartRanging(dev);
        if (status != 0) {  //failed to start polling
            return;
        }
        ranging_started = true;
    }
    
    status = VL53L4CD_CheckForDataReady(dev, &isReady);
    // printf("CheckForDataReady: status=%d, isReady=%d\r\n", status, isReady);

    if (status != 0) {
        return;
    }
    
    if (isReady)
    {
        // Clear HW interrupt to restart measurements
        VL53L4CD_ClearInterrupt(dev);
        
        // Read measured distance
        VL53L4CD_GetResult(dev, &results);
        // printf("Result: status=%d, range_status=%d, distance=%d mm\r\n", 
            //    status, results.range_status, results.distance_mm);

        if (results.range_status == 0  && results.distance_mm <= MEASUREMENT_THRESHOLD_MM) {
            //Detected within the threshold -- Send uart signal to send pause screen
            // printf("Detected Movement!\r\n");
            uart_send_string("PAUSE\n");
            
        }

        else if ((results.range_status == 0 || results.range_status == 2) && results.distance_mm >= MEASUREMENT_THRESHOLD_MM) {
            //No longer detecting obstacle -- sens uart signal to restart game
            // printf("No Movement Detected\r\n");
            // printf("Result: status=%d, range_status=%d, distance=%d mm\r\n", 
            //    status, results.range_status, results.distance_mm);
               uart_send_string("UNPAUSE\n");
        }
        
           
    }    
}

cy_rslt_t task_ir_init(void)
{
    //Add device initialization
    dev = 0x29;
    cy_rslt_t rslt;

    rslt = i2c_init(MODULE_SITE_1);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return -1;  //I2C not init
    }

    rslt = cyhal_gpio_init(MOD_2_PIN_IO_0, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);  
    if (rslt != CY_RSLT_SUCCESS) {
        return CY_RSLT_TYPE_ERROR;
    }
    // cyhal_system_delay_ms(100);

    rslt = cyhal_gpio_init(MOD_2_PIN_IO_1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_STRONG, 0);  
    if (rslt != CY_RSLT_SUCCESS) {
        return CY_RSLT_TYPE_ERROR;
    }
    // cyhal_system_delay_ms(100);

    cyhal_gpio_write(MOD_2_PIN_IO_0, 1);

    status = VL53L4CD_GetSensorId(dev, &sensor_id);

    status = VL53L4CD_SensorInit(dev);
    if (status == 0) {
        sensor_initialized = true;
    }

    status = VL53L4CD_SetRangeTiming(dev, 50, 0);
    if (status != 0) {
        return CY_RSLT_TYPE_ERROR;
    }

    return CY_RSLT_SUCCESS;

}