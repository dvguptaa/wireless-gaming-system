#ifndef __UART_H__
#define __UART_H__

#include "main.h"

#define UART_BAUD_RATE              115200
#define UART_DATA_BITS              8
#define UART_STOP_BITS              1
#define UART_PARITY                 CYHAL_UART_PARITY_NONE

#define UART_RX_PIN                 P5_0 
#define UART_TX_PIN                 P5_1

#define UART_INT_PRIORITY           4

#define UART_RX_BUFFER              256
#define UART_TX_BUFFER              256

#define UART_RX_QUEUE               10
#define UART_TX_QUEUE               32

#define UART_MSG_LENGTH             128


typedef struct 
{
    uint8_t *data;
    uint16_t length;
    bool requires_free;
} uart_message_t;

typedef void (*uart_rx_callback_t)(uint8_t *data, uint16_t length);

extern cyhal_uart_t uart_obj;

/* Queue Handles */
extern QueueHandle_t q_uart_rx;
extern QueueHandle_t q_uart_tx;

/* Task Handle */
extern TaskHandle_t Task_UART_Rx_Handle;

cy_rslt_t uart_hw_init(void);

cy_rslt_t task_uart_init(void);

void task_uart_tx(void *param);

void task_uart_rx(void *param);

BaseType_t uart_send(const uint8_t *data, uint16_t length);

BaseType_t uart_send_string(const char *str);

BaseType_t uart_printf(const char *format, ...);

void uart_register_rx_callback(uart_rx_callback_t callback);

void uart_event_handler(void *handler_arg, cyhal_uart_event_t event);

void uart_flush_rx(void);
void uart_flush_tx(void);


#endif