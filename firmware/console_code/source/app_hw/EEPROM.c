#include "EEPROM.h"
#include "i2c.h"

cy_rslt_t init_EEPROM(void) {

    cy_rslt_t rslt;
    rslt = cyhal_gpio_init(EEPROM_W_PIN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    eeprom_write(0xFF, 0x01);  // Initialize it with 0 as high score

    return rslt; /* Halt MCU if init fails*/

}

void eeprom_write(uint8_t data, uint8_t address) {

    //Need to enable CS write
    //Write the data to the eeprom
    uint8_t write_buffer[2] = {address, data};

    cyhal_gpio_write(EEPROM_W_PIN, 0);

    cyhal_i2c_master_write(&i2c_master_obj2, EEPROM_SUBORDINATE_ADDR, write_buffer, 2, 0, true);
    
    cyhal_gpio_write(EEPROM_W_PIN, 1);
}


uint8_t eeprom_read(uint8_t address) {


    uint8_t receive_data = 0x00;

    cyhal_i2c_master_write(&i2c_master_obj2, EEPROM_SUBORDINATE_ADDR, &address, 1, 0, false);

    cyhal_i2c_master_read(&i2c_master_obj2, EEPROM_SUBORDINATE_ADDR, &receive_data, 1, 0, true);
    
    return receive_data;

}