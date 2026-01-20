#include "main.h"

#ifndef __EEPROM_H__
#define __EEPROM_H__


#define EEPROM_SUBORDINATE_ADDR 0x51
#define EEPROM_W_PIN            P7_1

cy_rslt_t init_EEPROM(void);
void eeprom_write(uint8_t data, uint8_t address);
uint8_t eeprom_read(uint8_t address);

#endif