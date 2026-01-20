/**
 * @file cli_uart_commands.c
 * @brief CLI commands for UART communication with Raspberry Pi
 * 
 * Provides CLI commands to send data to the Raspberry Pi through UART:
 * - uart <message>  : Send a message to the Pi
 * - uartln <message>: Send a message with newline to the Pi
 * - uarttest        : Send a test message to verify connection
 */

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cli_uart_commands.h"
#include "uart.h"
#include "task_console.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/

static BaseType_t cli_handler_uart(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

static BaseType_t cli_handler_uarttest(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

/* CLI command definition for 'uart' command */

/* CLI command definition for 'uartln' command */
static const CLI_Command_Definition_t xUart =
{
    "uart",                                    /* command text */
    "\r\nuart <message>\r\n  Send message with newline to Raspberry Pi\r\n", /* help text */
    cli_handler_uart,                          /* handler function */
    2                                           /* Variable number of parameters */
};

/* CLI command definition for 'uarttest' command */
static const CLI_Command_Definition_t xUarttest =
{
    "uarttest",                                  /* command text */
    "\r\nuarttest\r\n  Send test message to Raspberry Pi\r\n", /* help text */
    cli_handler_uarttest,                        /* handler function */
    0                                            /* 0 parameters */
};

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
/**
 * @brief CLI handler for 'uartln' command
 * 
 * Sends a message to the Raspberry Pi via UART with a newline at the end.
 * Usage: uartln <message>
 */
static BaseType_t cli_handler_uart(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *pcParameter1, *pcParameter2;
    BaseType_t xParameterStringLength1, xParameterStringLength2;
    BaseType_t xReturn;
    BaseType_t result;
    char message_buffer[128];
    int int_value;

    /* Remove compile time warnings */
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    /* Obtain first parameter (string) */
    pcParameter1 = FreeRTOS_CLIGetParameter(
        pcCommandString,
        1,
        &xParameterStringLength1
    );
    configASSERT(pcParameter1);

    /* Obtain second parameter (int) */
    pcParameter2 = FreeRTOS_CLIGetParameter(
        pcCommandString,
        2,
        &xParameterStringLength2
    );
    configASSERT(pcParameter2);

    /* Validate lengths */
    if (xParameterStringLength1 == 0 || xParameterStringLength1 > 127)
    {
        sprintf(pcWriteBuffer, "Error: Invalid string length\r\n");
        return pdFALSE;
    }

    /* Copy string parameter */
    memset(message_buffer, 0x00, sizeof(message_buffer));
    strncpy(message_buffer, pcParameter1, xParameterStringLength1);
    message_buffer[xParameterStringLength1] = '\0';

    /* Convert integer parameter */
    int_value = atoi(pcParameter2);

    /* Format combined message */
    char combined_buffer[160];
    snprintf(combined_buffer, sizeof(combined_buffer),
             "%s %d\r\n", message_buffer, int_value);

    /* Send via UART */
    result = uart_send_string(combined_buffer);

    if (result == pdPASS)
    {
        //task_print_info("Sent to Pi: %s", combined_buffer);
    }

    /* Provide feedback to console */
    // memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    // if (result == pdPASS)
    // {
    //     snprintf(pcWriteBuffer, xWriteBufferLen,
    //              "Sent to Pi: %s", combined_buffer);
    // }
    else
    {
        task_print_warning("Error: Failed to send message\r\n");
    }

    /* Command complete */
    xReturn = pdFALSE;
    return xReturn;
}


/**
 * @brief CLI handler for 'uarttest' command
 * 
 * Sends a predefined test message to the Raspberry Pi to verify connection.
 * Usage: uarttest
 */
static BaseType_t cli_handler_uarttest(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    BaseType_t xReturn;
    BaseType_t result;
    const char *test_message = "PSoC Test Message - Connection OK!\r\n";

    /* Remove compile time warnings */
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    /* Send test message via UART */
    result = uart_send_string(test_message);

    /* Provide feedback to console */
    if (result == pdPASS)
    {
        task_print_info("Test message sent to Raspberry Pi\r\n");
        //sprintf(pcWriteBuffer, "Test message sent to Raspberry Pi\r\n");
    }
    else
    {
        task_print_info("Error: Failed to send test message\r\n");
        //sprintf(pcWriteBuffer, "Error: Failed to send test message\r\n");
    }

    /* Command complete */
    xReturn = pdFALSE;
    return xReturn;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief Initialize UART CLI commands
 * 
 * Registers all UART-related CLI commands with FreeRTOS-CLI.
 * Should be called after uart_init() in main.c
 */
cy_rslt_t cli_uart_commands_init(void)
{
    cy_rslt_t rslt;

    /* Register CLI commands */
    rslt = FreeRTOS_CLIRegisterCommand(&xUart);
    if (rslt != pdPASS) return rslt;

    rslt = FreeRTOS_CLIRegisterCommand(&xUarttest);
    if (rslt != pdPASS) return rslt;
    
    /* Log successful initialization */
    return CY_RSLT_SUCCESS;
}