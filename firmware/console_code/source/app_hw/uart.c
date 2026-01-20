#include "uart.h"
#include "task_console.h"
#include "i2c.h"

cyhal_uart_t uart_obj;

const cyhal_uart_cfg_t uart_config = 
{
    .data_bits = UART_DATA_BITS,
    .stop_bits = UART_STOP_BITS,
    .parity = UART_PARITY,
    .rx_buffer = NULL,
    .rx_buffer_size = 0
};

QueueHandle_t q_uart_rx = NULL;
QueueHandle_t q_uart_tx = NULL;

TaskHandle_t Task_UART_Rx_Handle = NULL;

static uint8_t uart_rx_buffer[UART_RX_BUFFER];
static uint16_t uart_rx_index = 0;

static uart_rx_callback_t rx_callback = NULL;

void uart_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    cy_rslt_t status;
    uint8_t c;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if ((event & CYHAL_UART_IRQ_TX_ERROR) == CYHAL_UART_IRQ_TX_ERROR)
    {
        /* TX Error occurred */
        // printf("UART TX Error\r\n");
    }
    else if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        /* Receive character */
        status = cyhal_uart_getc(&uart_obj, &c, 0);
        
        if (status == CY_RSLT_SUCCESS)
        {
            /* Store received character in buffer */
            if (uart_rx_index < UART_RX_BUFFER)
            {
                uart_rx_buffer[uart_rx_index++] = c;
                
                /* Check for end of line characters */
                if (c == '\n')
                {
                    /* Notify receive task that data is ready */
                    vTaskNotifyGiveFromISR(Task_UART_Rx_Handle, &xHigherPriorityTaskWoken);
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
            }
            else
            {
                /* Buffer overflow - reset buffer and notify task */
                // printf("UART RX buffer overflow\r\n");
                uart_rx_index = 0;
                vTaskNotifyGiveFromISR(Task_UART_Rx_Handle, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
}

cy_rslt_t uart_hw_init(void)
{
    cy_rslt_t rslt;
    uint32_t actual_baud;
    

    /* Initialize UART */
    rslt = cyhal_uart_init(
        &uart_obj,
        UART_TX_PIN,
        UART_RX_PIN,
        NC,
        NC,
        NULL,
        &uart_config);
    
    if (rslt != CY_RSLT_SUCCESS)
    {
        // printf("UART initialization failed\r\n");
        return rslt;
    }

    /* Set baud rate */
    rslt = cyhal_uart_set_baud(&uart_obj, UART_BAUD_RATE, &actual_baud);
    
    if (rslt != CY_RSLT_SUCCESS)
    {
        // printf("UART baud rate setting failed\r\n");
        return rslt;
    }

    uart_flush_rx();

    /* Register callback */
    cyhal_uart_register_callback(&uart_obj, uart_event_handler, NULL);

    /* Enable receive interrupts */
    cyhal_uart_enable_event(
        &uart_obj,
        (cyhal_uart_event_t)(CYHAL_UART_IRQ_RX_NOT_EMPTY),
        UART_INT_PRIORITY,
        true);
    
    return CY_RSLT_SUCCESS;
}

void task_uart_tx(void *param)
{
    uart_message_t msg;
    BaseType_t rtos_result;

    (void)param;

    for (;;)
    {
        /* Wait for message in queue */
        rtos_result = xQueueReceive(q_uart_tx, &msg, portMAX_DELAY);

        if (rtos_result == pdPASS)
        {
            /* Transmit data */
            for (uint16_t i = 0; i < msg.length; i++)
            {
                cyhal_uart_putc(&uart_obj, msg.data[i]);
            }

            // printf("Sent to Pi: %.*s\r\n", msg.length, msg.data);

            /* Free buffer if required */
            if (msg.requires_free)
            {
                vPortFree(msg.data);
            }
        }
    }
}

void task_uart_rx(void *param)
{
    uint8_t *data_copy;

    (void)param;

    for (;;)
    {
        /* Wait for notification from ISR */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (uart_rx_index > 0)
        {
            /* Allocate memory for received data */
            data_copy = pvPortMalloc(uart_rx_index + 1);

            if (data_copy != NULL)
            {
                /* Copy received data and null-terminate */
                memcpy(data_copy, uart_rx_buffer, uart_rx_index);
                data_copy[uart_rx_index] = '\0';

                /* Call registered callback if exists */
                if (rx_callback != NULL)
                {
                    rx_callback(data_copy, uart_rx_index);
                }

                /* Display received data directly to console - no queue needed */
                // printf("UART RX (%d bytes): %.*s\r\n", uart_rx_index, uart_rx_index, data_copy);

                /* Free the allocated memory */
                vPortFree(data_copy);
            }
            else
            {
                // printf("UART RX memory allocation failed\r\n");
            }

            /* Reset receive buffer */
            uart_rx_index = 0;
            memset(uart_rx_buffer, 0, UART_RX_BUFFER);
        }
    }
}

BaseType_t uart_send(const uint8_t *data, uint16_t length)
{
    uart_message_t msg;
    uint8_t *buffer;

    if (data == NULL || length == 0)
    {
        return pdFAIL;
    }

    /* Allocate buffer for data */
    buffer = pvPortMalloc(length);

    if (buffer == NULL)
    {
        return pdFAIL;
    }

    /* Copy data to buffer */
    memcpy(buffer, data, length);

    /* Prepare message */
    msg.data = buffer;
    msg.length = length;
    msg.requires_free = true;

    /* Send to transmit queue */
    if (xQueueSendToBack(q_uart_tx, &msg, 0) != pdPASS)
    {
        /* Queue full - flush it and try again */
        uart_flush_tx();
        
        if (xQueueSendToBack(q_uart_tx, &msg, 0) != pdPASS)
        {
            /* Still failed - give up */
            vPortFree(buffer);
            return pdFAIL;
        }
    }

    return pdPASS;
}

BaseType_t uart_send_string(const char *str)
{
    if (str == NULL)
    {
        return pdFAIL;
    }

    return uart_send((const uint8_t *)str, strlen(str));
}

BaseType_t uart_printf(const char *format, ...)
{
    char *buffer;
    va_list args;
    BaseType_t result;

    /* Allocate buffer */
    buffer = pvPortMalloc(UART_MSG_LENGTH);

    if (buffer == NULL)
    {
        return pdFAIL;
    }

    /* Format string */
    va_start(args, format);
    vsnprintf(buffer, UART_MSG_LENGTH, format, args);
    va_end(args);

    /* Send string */
    result = uart_send_string(buffer);

    /* Free buffer */
    vPortFree(buffer);

    return result;
}

void uart_register_rx_callback(uart_rx_callback_t callback)
{
    rx_callback = callback;
}

cy_rslt_t task_uart_init(void)
{

    cy_rslt_t rslt;

    /* Initialize hardware */
    rslt = uart_hw_init();
    
    if (rslt != CY_RSLT_SUCCESS)
    {
        // printf("UART hardware initialization failed\r\n");
        return rslt;
    }

    /* Create queues - removed q_uart_rx since we don't need it */
    q_uart_tx = xQueueCreate(UART_TX_QUEUE, sizeof(uart_message_t));

    if (q_uart_tx == NULL)
    {
        // printf("UART queue creation failed\r\n");
        return 0;
    }

    uart_flush_tx();

    /* Create transmit task */
    rslt = xTaskCreate(
            task_uart_tx,
            "UART TX",
            configMINIMAL_STACK_SIZE * 2,
            NULL,
            configMAX_PRIORITIES - 5,
            NULL);
    
    if (rslt != pdPASS)
    {
        // printf("UART TX task creation failed\r\n");
        return rslt;
    }

    /* Create receive task */
    rslt = xTaskCreate(
            task_uart_rx,
            "UART RX",
            configMINIMAL_STACK_SIZE * 2,
            NULL,
            configMAX_PRIORITIES - 5,
            &Task_UART_Rx_Handle);
    
    if  (rslt != pdPASS)
    {
        // printf("UART RX task creation failed\r\n");
        return rslt;
    }

    return CY_RSLT_SUCCESS;
}

void uart_flush_rx(void)
{
    uint8_t dummy;
    
    // Clear any pending data in UART hardware FIFO
    while (cyhal_uart_readable(&uart_obj) > 0)
    {
        cyhal_uart_getc(&uart_obj, &dummy, 0);
    }
    
    // Clear software buffer
    uart_rx_index = 0;
    memset(uart_rx_buffer, 0, UART_RX_BUFFER);
    
    // printf("UART RX buffer flushed\r\n");
}


void uart_flush_tx(void)
{
    uart_message_t msg;
    
    /* Drain the queue and free any allocated buffers */
    while (xQueueReceive(q_uart_tx, &msg, 0) == pdPASS)
    {
        if (msg.requires_free && msg.data != NULL)
        {
            vPortFree(msg.data);
        }
    }
}