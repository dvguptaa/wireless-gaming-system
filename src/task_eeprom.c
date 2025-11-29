
#include "task_eeprom.h"
#include "main.h"
#include "eeprom.h"
#include "task_console.h"
#include "FreeRTOS_CLI.h"

static cyhal_spi_t *eeprom_spi_obj = NULL;
static cyhal_gpio_t eeprom_cs_pin = NC;

QueueHandle_t Queue_EEPROM_Requests;

/* Forward declarations */
static BaseType_t cli_handler_eeprom(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/* CLI command definition */
static const CLI_Command_Definition_t cmd_eeprom = {
    "eeprom",
    "\r\neeprom <read|write> <addr> [val]\r\n",
    cli_handler_eeprom,
    -1
};

/* ============================= CLI Handler ============================= */
static BaseType_t cli_handler_eeprom(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *param;
    BaseType_t param_len;
    char *end_ptr;
    
    /* *******************************************************************
     * ** FIX: Create a local buffer to safely copy and null-terminate
     * ** parameters before passing them to strtol.
     * ** A 16-char buffer is more than enough for hex addresses/values.
     * *******************************************************************
     */
    char param_buffer[16];

    eeprom_message_t request;
    eeprom_message_t response;
    QueueHandle_t rsp_queue = NULL;

    // Clear output buffer
    memset(pcWriteBuffer, 0, xWriteBufferLen);

    // Get first parameter (read/write)
    param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_len);
    if (param == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Missing command (read/write)\r\n");
        return pdFALSE;
    }

    if (strncmp(param, "read", param_len) == 0) {
        // Get address
        param = FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_len);
        if (!param) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Missing address\r\n");
            return pdFALSE;
        }
        
        /* FIX: Safely copy parameter to local buffer for strtol */
        if (param_len >= sizeof(param_buffer)) {
             snprintf(pcWriteBuffer, xWriteBufferLen, "Address too long\r\n");
            return pdFALSE;
        }
        memset(param_buffer, 0, sizeof(param_buffer));
        strncpy(param_buffer, param, param_len);
        /* strtol converts string to long, base 16 */
        uint16_t addr = (uint16_t)strtol(param_buffer, &end_ptr, 16);
        if (*end_ptr != '\0') {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid address format\r\n");
            return pdFALSE;
        }

        // Create response queue
        rsp_queue = xQueueCreate(1, sizeof(eeprom_message_t));
        if (rsp_queue == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to create response queue\r\n");
            return pdFALSE;
        }

        request.command = EEPROM_CMD_READ_DATA;
        request.address = addr;
        request.length = 1;
        request.data = NULL;
        request.response_queue = rsp_queue;

        if (xQueueSend(Queue_EEPROM_Requests, &request, pdMS_TO_TICKS(1000)) != pdPASS) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to send EEPROM request\r\n");
            vQueueDelete(rsp_queue);
            return pdFALSE;
        }

        if (xQueueReceive(rsp_queue, &response, pdMS_TO_TICKS(5000)) != pdPASS) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "EEPROM read timeout\r\n");
            vQueueDelete(rsp_queue);
            return pdFALSE;
        }

        snprintf(pcWriteBuffer, xWriteBufferLen, "EEPROM[0x%04X] = 0x%02X\r\n", 
                addr, response.data[0]);
        
        // Clean up
        if (response.data) {
            vPortFree(response.data);
        }
        vQueueDelete(rsp_queue);
    }
    else if (strncmp(param, "write", param_len) == 0) {
        // Get address
        param = FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_len);
        if (!param || param_len <= 0) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Missing address\r\n");
            return pdFALSE;
        }
        
        /* FIX: Safely copy parameter to local buffer for strtol */
        if (param_len >= sizeof(param_buffer)) {
             snprintf(pcWriteBuffer, xWriteBufferLen, "Address too long\r\n");
            return pdFALSE;
        }
        memset(param_buffer, 0, sizeof(param_buffer));
        strncpy(param_buffer, param, param_len);
        /* strtol converts string to long, base 16 */
        long addr_l = strtol(param_buffer, &end_ptr, 16);
        if (*end_ptr != '\0' || addr_l < 0 || addr_l > 0xFFFF) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid address format\r\n");
            return pdFALSE;
        }
        uint16_t addr = (uint16_t)addr_l;


        // Get value
        param = FreeRTOS_CLIGetParameter(pcCommandString, 3, &param_len);
        if (!param || param_len <= 0) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Missing value\r\n");
            return pdFALSE;
        }

        /* FIX: Safely copy parameter to local buffer for strtol */
        if (param_len >= sizeof(param_buffer)) {
             snprintf(pcWriteBuffer, xWriteBufferLen, "Value too long\r\n");
            return pdFALSE;
        }
        memset(param_buffer, 0, sizeof(param_buffer));
        strncpy(param_buffer, param, param_len);
        /* strtol converts string to long, base 16 */
        long val_l = strtol(param_buffer, &end_ptr, 16);
        if (*end_ptr != '\0' || val_l < 0 || val_l > 0xFF) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid value format (must be 0x00-0xFF)\r\n");
            return pdFALSE;
        }
        uint8_t value = (uint8_t)val_l;


        request.command = EEPROM_CMD_WRITE_DATA;
        request.address = addr;
        request.length = 1;
        request.data = pvPortMalloc(1);
        if (request.data == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Memory allocation failed\r\n");
            return pdFALSE;
        }
        request.data[0] = value;
        request.response_queue = NULL;

        if (xQueueSend(Queue_EEPROM_Requests, &request, pdMS_TO_TICKS(1000)) != pdPASS) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to send EEPROM request\r\n");
            vPortFree(request.data);
            return pdFALSE;
        }

        snprintf(pcWriteBuffer, xWriteBufferLen, "Wrote 0x%02X to EEPROM[0x%04X]\r\n", 
                value, addr);
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid command. Use 'read' or 'write'.\r\n");
    }

    return pdFALSE;
}

/* ============================= Task Init ============================= */
bool task_eeprom_resources_init(SemaphoreHandle_t *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    if (spi_semaphore == NULL || spi_obj == NULL || cs_pin == NC) {
        return false;
    }

    eeprom_spi_obj = spi_obj;
    eeprom_cs_pin = cs_pin;

    // Initialize EEPROM driver
    eeprom_init(eeprom_spi_obj, eeprom_cs_pin);

    // Create request queue
    Queue_EEPROM_Requests = xQueueCreate(5, sizeof(eeprom_message_t));
    if (Queue_EEPROM_Requests == NULL) {
        return false;
    }

    // Register CLI command
    FreeRTOS_CLIRegisterCommand(&cmd_eeprom);

    // Create task
    if (xTaskCreate(
            task_eeprom,
            "EEPROM",
            5 * configMINIMAL_STACK_SIZE,
            spi_semaphore,
            tskIDLE_PRIORITY + 1,
            NULL) != pdPASS) {
        return false;
    }
    
    return true;
}

/* ============================= EEPROM Task ============================= */
void task_eeprom(void *arg)
{
    SemaphoreHandle_t *Semaphore_EEPROM = NULL;
    eeprom_message_t request;

    if (arg != NULL) {
        Semaphore_EEPROM = (SemaphoreHandle_t *)arg;
    } else {
        CY_ASSERT(0);
    }
    
    task_print_info("EEPROM task started successfully");

    while (1) {
        // Wait for a request to arrive
        if (xQueueReceive(Queue_EEPROM_Requests, &request, portMAX_DELAY) != pdPASS) {
            continue;
        }

        // Take the SPI semaphore
        if (xSemaphoreTake(*Semaphore_EEPROM, pdMS_TO_TICKS(5000)) != pdPASS) {
            task_print_error("Failed to take SPI semaphore");
            // Clean up request data if needed
            if (request.data && request.command == EEPROM_CMD_WRITE_DATA) {
                vPortFree(request.data);
            }
            continue;
        }

        // Process the request
        switch (request.command) {
        case EEPROM_CMD_WRITE_DATA:
            if (request.data != NULL) {
                for (uint8_t i = 0; i < request.length; i++) {
                    eeprom_write_byte(request.address + i, request.data[i]);
                }
                // Free data AFTER the loop
                vPortFree(request.data);
            }
            break;

        case EEPROM_CMD_READ_DATA:
            // Allocate memory if not provided
            if (request.data == NULL) {
                request.data = pvPortMalloc(request.length);
                if (request.data == NULL) {
                    task_print_error("Failed to allocate memory for EEPROM read");
                    xSemaphoreGive(*Semaphore_EEPROM);
                    continue;
                }
            }
            
            // Read data
            for (uint8_t i = 0; i < request.length; i++) {
                request.data[i] = eeprom_read_byte(request.address + i);
            }

            // Send response if requested
            if (request.response_queue != NULL) {
                if (xQueueSend(request.response_queue, &request, pdMS_TO_TICKS(1000)) != pdPASS) {
                    task_print_error("Failed to send EEPROM response");
                    vPortFree(request.data);
                }
            }
            break;

        default:
            task_print_error("Invalid EEPROM command");
            if (request.data) {
                vPortFree(request.data);
            }
            break;
        }

        // Release SPI semaphore
        xSemaphoreGive(*Semaphore_EEPROM);
    }
}
