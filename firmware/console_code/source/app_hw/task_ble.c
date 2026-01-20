#include "task_ble.h"

/* Include the main project header to pick up FreeRTOSConfig and Types */
#if __has_include("../../main.h")
    #include "../../main.h"
#else
    #include "main.h"
#endif

/* Cypress / Infineon BLE Stack Includes */
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cybt_platform_trace.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"
#include "console.h"
#include "timer.h"

#include <string.h>
#include <stdio.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define TARGET_DEVICE_NAME      "Controller"    /* Name of the BLE server device to connect to */
#define SERVER_CCCD_HANDLE      0x000E          /* Handle for Notifications (IMU) */
#define HAPTIC_HANDLE           0x000C          /* Handle for DC Motor/Haptics */

#define BLE_TASK_STACK_SIZE     (1024)          /* 4KB Stack */
#define BLE_TASK_PRIORITY       (configMAX_PRIORITIES - 2) 

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* CONTROL VARIABLE: 0=OFF, 1=ON 
 * Change this in the Debugger Watch Window to control the motor.
 */
volatile uint8_t ble_motor_request = 1;

/* Connection State */
static volatile uint16_t connection_id = 0;
static wiced_bt_device_address_t server_address = {0};
static bool server_found = false;

/* Data buffer for enabling notifications (0x01 = Enable, 0x00 = Disable) */
static uint8_t ble_notify_enable_data[2] = {0x01, 0x00};

/* Static buffer for Motor Commands to prevent memory crashes */
static uint8_t ble_motor_cmd_buffer = 0;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void ble_client_task_func(void *arg);
static wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                                        wiced_bt_management_evt_data_t *p_event_data);
static wiced_bt_gatt_status_t app_gatt_callback(wiced_bt_gatt_evt_t event,
                                                wiced_bt_gatt_event_data_t *p_data);
static void scan_result_callback(wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data);

/*******************************************************************************
* Function Name: task_ble_init
*******************************************************************************/
void task_ble_init(void)
{
    BaseType_t rtos_result;

    rtos_result = xTaskCreate(ble_client_task_func, 
                              "BLE Client", 
                              BLE_TASK_STACK_SIZE, 
                              NULL, 
                              configMAX_PRIORITIES - 4, //FIx priority issue
                              NULL);

    // printf("Created BLE Client task\r\n");
    if (rtos_result != pdPASS)
    {
        // printf("Failed to create BLE Client task\r\n");
        CY_ASSERT(0);
    }
}

/*******************************************************************************
* Function Name: task_ble_send_motor_cmd
*******************************************************************************/
void task_ble_send_motor_cmd(uint8_t command)
{
    if (connection_id == 0) return; 

    wiced_bt_gatt_write_hdr_t write_hdr;

    /* Update the static buffer */
    ble_motor_cmd_buffer = command;

    memset(&write_hdr, 0, sizeof(write_hdr));
    write_hdr.handle = HAPTIC_HANDLE;
    write_hdr.len = 1;
    write_hdr.offset = 0;
    write_hdr.auth_req = GATT_AUTH_REQ_NONE;

    /* Send Write Request */
    wiced_bt_gatt_status_t status = wiced_bt_gatt_client_send_write(
        connection_id, 
        GATT_REQ_WRITE, 
        &write_hdr, 
        &ble_motor_cmd_buffer, 
        NULL 
    );

    if (status != WICED_BT_GATT_SUCCESS)
    {
        // printf("Failed to send motor command. Status: %d\r\n", status);
    }
}

/*******************************************************************************
* Function Name: ble_client_task_func
*******************************************************************************/
static void ble_client_task_func(void *arg)
{
    (void)arg;
    cy_rslt_t result;
    uint8_t current_request_state = 0;
    uint8_t last_request_state = 0;
    
    // printf("BLE Client task started\r\n");
    
    /* Initialize platform specific Bluetooth configuration */
    cybt_platform_config_init(&cybsp_bt_platform_cfg);

    /* Initialize the Bluetooth Stack */
    result = wiced_bt_stack_init(app_bt_management_callback, &wiced_bt_cfg_settings);
    if (WICED_BT_SUCCESS != result)
    {
        // printf("BT Stack init failed: %d\r\n", (int)result);
        CY_ASSERT(0);
    }
    
    for(;;)
    {
        current_request_state = ble_motor_request;

        /* Check if the variable has changed */
        if (current_request_state != last_request_state)
        {
            if (connection_id != 0)
            {
                // printf("Sending Motor Command: %d\r\n", current_request_state);
                task_ble_send_motor_cmd(current_request_state);
                last_request_state = current_request_state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*******************************************************************************
* Function Name: app_bt_management_callback
*******************************************************************************/
static wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                                        wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_bt_dev_status_t status = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = {0};

    switch (event)
    {
        case BTM_ENABLED_EVT:
            // printf("Bluetooth Enabled\r\n");
            wiced_bt_dev_read_local_addr(bda);
            wiced_bt_gatt_register(app_gatt_callback);
            // printf("Scanning for '%s'...\r\n", TARGET_DEVICE_NAME);
            wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, scan_result_callback);
            break;

        case BTM_DISABLED_EVT:
            // printf("Bluetooth Disabled\r\n");
            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
            break;

        case BTM_BLE_SCAN_STATE_CHANGED_EVT:
            break;

        default:
            // printf("BT Event: 0x%x\r\n", event);
            break;
    }

    return status;
}

/*******************************************************************************
* Function Name: scan_result_callback
*******************************************************************************/
static void scan_result_callback(wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data)
{
    uint8_t len = 0;
    uint8_t *p_name = NULL;
    
    if (p_scan_result == NULL)
    {
        if (!server_found)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, scan_result_callback);
        }
        return;
    }
    
    p_name = wiced_bt_ble_check_advertising_data(p_adv_data, BTM_BLE_ADVERT_TYPE_NAME_COMPLETE, &len);
    if (!p_name) p_name = wiced_bt_ble_check_advertising_data(p_adv_data, BTM_BLE_ADVERT_TYPE_NAME_SHORT, &len);
    
    if (p_name != NULL)
    {
        if (len == strlen(TARGET_DEVICE_NAME) && memcmp(p_name, TARGET_DEVICE_NAME, len) == 0)
        {
            // printf("Target found. Connecting...\r\n");
            server_found = true;
            memcpy(server_address, p_scan_result->remote_bd_addr, BD_ADDR_LEN);
            wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE, scan_result_callback);
            wiced_bt_gatt_le_connect(server_address, p_scan_result->ble_addr_type, BLE_CONN_MODE_HIGH_DUTY, WICED_TRUE);
        }
    }
}

/*******************************************************************************
* Function Name: app_gatt_callback
*******************************************************************************/
static wiced_bt_gatt_status_t app_gatt_callback(wiced_bt_gatt_evt_t event,
                                                wiced_bt_gatt_event_data_t *p_data)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    static bool timer_started = false;

    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            if (p_data->connection_status.connected)
            {
                connection_id = p_data->connection_status.conn_id;
                // printf("Connected (ID: %d). Enabling Notifications...\r\n", connection_id);
                xEventGroupSetBits(wall_event, CONNECTION_EVENT_BIT);//daksh change- connection sound

                wiced_bt_gatt_write_hdr_t write_hdr;
                memset(&write_hdr, 0, sizeof(write_hdr));
                write_hdr.handle    = SERVER_CCCD_HANDLE;
                write_hdr.len       = 2;
                write_hdr.auth_req  = GATT_AUTH_REQ_NONE;

                wiced_bt_gatt_client_send_write(connection_id, GATT_REQ_WRITE, &write_hdr, ble_notify_enable_data, NULL);
                if (!timer_started)
                {
                    // printf("Starting timer...\r\n");
                    timer_start();
                    timer_started = true;
                }
            }
            else
            {
                // printf("Disconnected (%d). Restarting scan...\r\n", p_data->connection_status.reason);
                connection_id = 0;
                server_found = false;
                wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE, scan_result_callback);
            }
            break;

        case GATT_OPERATION_CPLT_EVT:
            
            /* Check for Incoming Notifications */
            if (p_data->operation_complete.op == GATTC_OPTYPE_NOTIFICATION)
            {
                wiced_bt_gatt_data_t *p_notif = &p_data->operation_complete.response_data.att_value;
                
                /* CHANGED: Accept any length >= 1 byte */
                if (p_notif->p_data != NULL && p_notif->len >= 1)
                {
                    uint8_t gesture = p_notif->p_data[0];

                    static uint8_t left_count = 0;
                    static uint8_t right_count = 0;
                    static uint8_t up_count = 0;
                    static uint8_t down_count = 0;
                    int MOVE_THRESHOLD = 1;
                    switch (gesture)
                    {
                        case 0x01:  
                        left_count++;
                        if (left_count >= MOVE_THRESHOLD) {
                            uart_send_string("LEFT\n");
                            // printf("Move: LEFT\r\n");
                            left_count = 0;
                        }
                        right_count = 0;
                        up_count = 0;
                        down_count = 0;
                        break;
                        case 0x02:
                        right_count++;
                        if (right_count >= MOVE_THRESHOLD) {
                            // printf("Move: RIGHT\r\n"); 
                            uart_send_string("RIGHT\n");
                            right_count = 0;
                        }
                        left_count = 0;
                        up_count = 0;
                        down_count = 0;
                        break;
                        case 0x03: 
                        up_count++;
                        if (up_count >= MOVE_THRESHOLD) {
                            // printf("Move: UP\r\n"); 
                            uart_send_string("UP\n");
                            up_count = 0;
                        }
                        left_count = 0;
                        right_count = 0;
                        down_count = 0;
                        break;
                        case 0x04: 
                        down_count++;
                        if (down_count >= MOVE_THRESHOLD) {
                            // printf("Move: DOWN\r\n");
                            uart_send_string("DOWN\n"); 
                            down_count = 0;
                        }
                        left_count = 0;
                        right_count = 0;
                        up_count = 0;
                        break;
                        case 0x00: 
                        // printf("Idle\r\n"); 
                        break;
                        default:   break;
                    }
                }
            }
            break;

        default:
            break;
    }

    return status;
}
