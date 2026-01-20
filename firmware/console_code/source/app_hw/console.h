#include "main.h"
#include "ece453_pins.h"
#include "task_console.h"

#include "i2c.h"
#include "uart.h"
#include "EEPROM.h"
#include "light_sensor.h"
#include "Speakers.h"
#include "timer.h"
#include "IR.h"
#include "cli_uart_commands.h"


#ifndef __CONSOLE_H__
#define __CONSOLE_H__

int console_init();
void uart_score_callback(uint8_t *data, uint16_t length);
void EEPROM_task(void *param);
void LR_task(void *param);
void TOF_task(void *param);
void Speaker_task(void *param);


#endif