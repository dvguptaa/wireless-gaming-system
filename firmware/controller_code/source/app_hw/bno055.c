#include "bno055.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task_console.h"

static cyhal_uart_t *bno_uart;
static SemaphoreHandle_t bno_uart_mutex = NULL;

/* Debug control: set to 1 to enable very verbose per-byte TX/RX logs */
#ifndef BNO055_DEBUG
#define BNO055_DEBUG 0
#endif

/* Helper: Write a register via BNO055 UART Protocol */
static cy_rslt_t bno055_write_reg(uint8_t reg, uint8_t data)
{
    if (bno_uart == NULL) return CY_RSLT_TYPE_ERROR;

    if (bno_uart_mutex) xSemaphoreTake(bno_uart_mutex, portMAX_DELAY);

    cyhal_uart_clear(bno_uart);

    uint8_t packet[5];
    packet[0] = BNO_UART_START_BYTE;
    packet[1] = BNO_UART_WRITE;
    packet[2] = reg;
    packet[3] = 1;
    packet[4] = data;

    if (BNO055_DEBUG) task_print_info("BNO055: TX -> [ %02X %02X %02X %02X %02X ]", packet[0], packet[1], packet[2], packet[3], packet[4]);

    for (int i = 0; i < 5; ++i) {
        if (cyhal_uart_putc(bno_uart, packet[i]) != CY_RSLT_SUCCESS) {
            if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
            task_print_error("BNO055: UART TX failed sending byte %d", i);
            return CY_RSLT_TYPE_ERROR;
        }
    }

    uint8_t recv_buf[8];
    int recv_count = 0;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(200);
    while (xTaskGetTickCount() < deadline && recv_count < (int)sizeof(recv_buf)) {
        uint8_t b;
        cy_rslt_t r = cyhal_uart_getc(bno_uart, &b, 20);
        if (r == CY_RSLT_SUCCESS) {
            recv_buf[recv_count++] = b;
        }
    }

    if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);

    if (recv_count == 0) {
        task_print_error("BNO055: Write Timeout (Reg: 0x%02X) - no response", reg);
        return CY_RSLT_TYPE_ERROR;
    }

    /* Log received stream (verbose only) */
    if (BNO055_DEBUG) {
        char msg[80];
        int off = 0;
        off += snprintf(msg + off, sizeof(msg) - off, "BNO055: RX <-");
        for (int i = 0; i < recv_count; ++i) off += snprintf(msg + off, sizeof(msg) - off, " %02X", recv_buf[i]);
        task_print_info("%s", msg);
    }

    for (int i = 0; i < recv_count; ++i) {
        if (recv_buf[i] == 0xEE) {
            if (i+1 < recv_count) {
                if (recv_buf[i+1] == 0x01) return CY_RSLT_SUCCESS;
                else { task_print_warning("BNO055: ACK found but status != 0x01 (0x%02X)", recv_buf[i+1]); return CY_RSLT_TYPE_ERROR; }
            } else {
                return CY_RSLT_SUCCESS;
            }
        }
    }

    task_print_error("BNO055: ACK not found in response");
    return CY_RSLT_TYPE_ERROR;
}

/* Helper: Read registers */
static cy_rslt_t bno055_read_regs(uint8_t reg, uint8_t *buffer, uint8_t len)
{
    if (bno_uart == NULL || buffer == NULL) return CY_RSLT_TYPE_ERROR;

    if (bno_uart_mutex) xSemaphoreTake(bno_uart_mutex, portMAX_DELAY);

    cyhal_uart_clear(bno_uart);

    uint8_t cmd[4];
    cmd[0] = BNO_UART_START_BYTE;
    cmd[1] = BNO_UART_READ;
    cmd[2] = reg;
    cmd[3] = len;

    if (BNO055_DEBUG) task_print_info("BNO055: TX(Read) -> %02X %02X %02X %02X", cmd[0], cmd[1], cmd[2], cmd[3]);

    for (int i = 0; i < 4; ++i) {
        if (cyhal_uart_putc(bno_uart, cmd[i]) != CY_RSLT_SUCCESS) {
            if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
            return CY_RSLT_TYPE_ERROR;
        }
    }

    uint8_t header = 0;
    {
        const TickType_t hdr_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(750);
        bool got_header = false;
        int skipped_count = 0;
        while (xTaskGetTickCount() < hdr_deadline) {
            cy_rslt_t rh = cyhal_uart_getc(bno_uart, &header, 50);
            if (rh == CY_RSLT_SUCCESS) {
                if (header == 0xBB) { got_header = true; break; }
                skipped_count++;
                if (BNO055_DEBUG) task_print_info("BNO055: Skipped byte while waiting for header: 0x%02X", header);
                continue;
            }
        }
        if (!got_header) {
            //if (skipped_count > 0) task_print_warning("BNO055: Skipped %d bytes while waiting for header", skipped_count);
            if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
            //task_print_error("BNO055: No response header for read reg 0x%02X", reg);
            return CY_RSLT_TYPE_ERROR;
        }
        if (skipped_count > 0 && BNO055_DEBUG) task_print_info("BNO055: Found header after skipping %d bytes", skipped_count);
    }

    uint8_t reported_len = 0;
    if (cyhal_uart_getc(bno_uart, &reported_len, 20) != CY_RSLT_SUCCESS) {
        if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
        return CY_RSLT_TYPE_ERROR;
    }

    for (int i = 0; i < len; ++i) {
        uint8_t rx_byte = 0;
        if (cyhal_uart_getc(bno_uart, &rx_byte, 50) != CY_RSLT_SUCCESS) {
            if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
            return CY_RSLT_TYPE_ERROR;
        }
        buffer[i] = rx_byte;
    }

    if (bno_uart_mutex) xSemaphoreGive(bno_uart_mutex);
    if (BNO055_DEBUG) task_print_info("BNO055: Read resp: header=0x%02X len=%u data=%02X", header, reported_len, buffer[0]);
    return CY_RSLT_SUCCESS;
}

cy_rslt_t bno055_init(cyhal_uart_t *uart_obj)
{
    bno_uart = uart_obj;
    uint8_t chip_id = 0;
    cy_rslt_t rs;

    if (bno_uart_mutex == NULL) {
        bno_uart_mutex = xSemaphoreCreateMutex();
        if (bno_uart_mutex == NULL) return CY_RSLT_TYPE_ERROR;
    }

    // 1. Verify Chip ID
    task_print_info("BNO055: Verifying Chip ID...");
    bool id_found = false;
    for (int i = 0; i < 4; i++) {
        rs = bno055_read_regs(BNO055_CHIP_ID_ADDR, &chip_id, 1);
        if (rs == CY_RSLT_SUCCESS && chip_id == 0xA0) {
            id_found = true;
            task_print_info("BNO055: Chip ID OK (0xA0)");
            break;
        }
        cyhal_system_delay_ms(200);
    }

    if (!id_found) {
        task_print_error("BNO055: Failed to read ID (Got 0x%02X)", chip_id);
        return CY_RSLT_TYPE_ERROR;
    }

    cyhal_system_delay_ms(50); 

    // 2. Enter CONFIG mode
    task_print_info("BNO055: Setting CONFIG Mode...");
    rs = bno055_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_CONFIG);
    if (rs != CY_RSLT_SUCCESS) return rs;
    
    cyhal_system_delay_ms(50); 

    // 3. SKIP EXTERNAL CRYSTAL (Internal Oscillator)
    task_print_info("BNO055: Using Internal Oscillator");

    // 4. Set NDOF Mode
    task_print_info("BNO055: Setting NDOF Mode...");
    rs = bno055_write_reg(BNO055_OPR_MODE_ADDR, OPERATION_MODE_NDOF);
    if (rs != CY_RSLT_SUCCESS) { 
        task_print_error("BNO055: Failed to set NDOF mode");
        return rs;
    } 
    
    cyhal_system_delay_ms(50); 

    return CY_RSLT_SUCCESS;
}

cy_rslt_t bno055_read_euler(bno055_vec3_t *euler)
{
    uint8_t buf[6];
    cy_rslt_t rs = bno055_read_regs(BNO055_EULER_H_LSB_ADDR, buf, 6);
    if (rs != CY_RSLT_SUCCESS) return rs;

    euler->x = (int16_t)((buf[1] << 8) | buf[0]);
    euler->y = (int16_t)((buf[3] << 8) | buf[2]);
    euler->z = (int16_t)((buf[5] << 8) | buf[4]);

    return CY_RSLT_SUCCESS;
}

