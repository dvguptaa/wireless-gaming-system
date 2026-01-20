/**
 * @file cli_uart_commands.h
 * @brief CLI commands for UART communication with Raspberry Pi
 * 
 * This file provides CLI commands that allow sending data to the
 * Raspberry Pi through the UART interface. Commands can be entered
 * through the console to transmit messages to the Pi.
 */

#ifndef __CLI_UART_COMMANDS_H__
#define __CLI_UART_COMMANDS_H__

/*******************************************************************************
 * Header files
 ******************************************************************************/
#include "main.h"

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize UART CLI commands
 * 
 * Registers all UART-related CLI commands with FreeRTOS-CLI.
 * This function should be called after uart_init() in main.c
 */
cy_rslt_t cli_uart_commands_init(void);

#endif /* __CLI_UART_COMMANDS_H__ */