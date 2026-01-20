///newest code//////
#include "main.h"
#include "task_bluetooth.h"
#include "task_imu.h"
#include "task_motor.h"
#include "task_button.h"
#include "task_console.h"

/* Stack Includes */
#include "cybsp.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_gatt.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"
#include "cycfg_gatt_db.h"

/* --- TIMING CONFIGURATION --- */
#define LOOP_DELAY_MS       50      /* Task update rate (20Hz) */

/* ADJUSTED SPEED: 350ms = approx 2.8 moves per second */
#define REPEAT_DELAY_MS     275   

/* Convert to ticks */
#define REPEAT_TICKS        (REPEAT_DELAY_MS / LOOP_DELAY_MS)

#define CMD_ID_MOTOR        0x01
#define CMD_ID_CALIB        0x02

static uint16_t connection_id = 0;
static bool notify_enabled = false;

/* State Tracking */
static imu_gesture_t last_gesture_state = GESTURE_NONE;
static uint32_t hold_timer = 0;

/* Prototypes */
static void ble_task(void *arg);
static void send_notification(uint8_t gesture_val);
static wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data);
static wiced_bt_gatt_status_t app_gatt_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_data);
static wiced_bt_gatt_status_t app_gatt_attr_write_handler(uint16_t conn_id, wiced_bt_gatt_opcode_t opcode, wiced_bt_gatt_write_req_t *p_write_req);
static wiced_bt_gatt_status_t app_gatt_attr_read_handler(uint16_t conn_id, wiced_bt_gatt_opcode_t opcode, wiced_bt_gatt_read_t *p_read_req, uint16_t len_requested);

void task_bluetooth_init(void) {
    xTaskCreate(ble_task, "BLE Task", 4096, NULL, configMAX_PRIORITIES - 2, NULL);
}

void ble_task(void *arg)
{
    (void)arg;
    cybt_platform_config_init(&cybsp_bt_platform_cfg);
    wiced_bt_stack_init(app_bt_management_callback, &wiced_bt_cfg_settings);

    for(;;)
    {
        /* Only run logic if connected and enabled */
        if (connection_id != 0 && notify_enabled)
        {
            imu_data_t data;
            task_imu_get_data(&data);

            /* GESTURE LOGIC */
            if (data.gesture != GESTURE_NONE)
            {
                /* CASE 1: New Gesture (Rising Edge) */
                if (data.gesture != last_gesture_state)
                {
                    /* Send IMMEDIATE 1st packet */
                    send_notification((uint8_t)data.gesture);
                    
                    /* Reset timer */
                    hold_timer = 0; 
                }
                /* CASE 2: Holding Same Gesture */
                else
                {
                    hold_timer++;
                    
                    /* Check if we reached the CONSTANT threshold */
                    if (hold_timer >= REPEAT_TICKS)
                    {
                        /* Send Repeat Packet */
                        send_notification((uint8_t)data.gesture);
                        
                        /* Reset timer */
                        hold_timer = 0;
                    }
                }
            }
            else
            {
                /* Idle: Reset everything */
                hold_timer = 0;
            }
            
            /* Update History */
            last_gesture_state = data.gesture;
        }
        else
        {
            /* Not connected: Reset state */
            last_gesture_state = GESTURE_NONE;
            hold_timer = 0;
        }
        
        /* Loop Delay (Frequency of checks) */
        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
    }
}

static void send_notification(uint8_t gesture_val)
{
    uint8_t packet[1] = { gesture_val };
    wiced_bt_gatt_server_send_notification(connection_id, HDLC_SENSOR_DATA_VALUE, 1, packet, NULL);
}

/* --- BOILERPLATE CALLBACKS (Unchanged) --- */

static wiced_bt_gatt_status_t app_gatt_attr_write_handler(uint16_t conn_id, wiced_bt_gatt_opcode_t opcode, wiced_bt_gatt_write_req_t *p_write_req)
{
    if (p_write_req->handle == HDLD_SENSOR_DATA_CLIENT_CHAR_CONFIG) {
        notify_enabled = (p_write_req->p_val[0] & GATT_CLIENT_CONFIG_NOTIFICATION) ? true : false;
        task_print_info("BLE: Notifications %s", notify_enabled ? "Enabled" : "Disabled");
    }
    else if (p_write_req->handle == HDLC_SENSOR_COMMAND_VALUE) {
        if (p_write_req->val_len == 1 && p_write_req->p_val[0] == 1) {
            motor_message_t msg = {MOTOR_PULSE_START, 100, 500};
            if(q_haptics) xQueueSendToBack(q_haptics, &msg, 0);
        }
        else if (p_write_req->val_len >= 4 && p_write_req->p_val[0] == CMD_ID_MOTOR) {
            uint16_t dur = p_write_req->p_val[2] | (p_write_req->p_val[3] << 8);
            motor_message_t msg = {MOTOR_PULSE_START, p_write_req->p_val[1], dur};
            if(q_haptics) xQueueSendToBack(q_haptics, &msg, 0);
        }
        else if (p_write_req->p_val[0] == CMD_ID_CALIB) {
            task_imu_req_calibration();
        }
    }
    if (opcode == GATT_REQ_WRITE) wiced_bt_gatt_server_send_write_rsp(conn_id, opcode, p_write_req->handle);
    return WICED_BT_GATT_SUCCESS;
}

static wiced_bt_dev_status_t app_bt_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data) {
    if(event == BTM_ENABLED_EVT) {
        wiced_bt_gatt_register(app_gatt_callback);
        wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
        wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE, cy_bt_adv_packet_data);
        wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, BLE_ADDR_PUBLIC, NULL);
    }
    return WICED_BT_SUCCESS;
}

static wiced_bt_gatt_status_t app_gatt_callback(wiced_bt_gatt_evt_t event, wiced_bt_gatt_event_data_t *p_data) {
    if (event == GATT_CONNECTION_STATUS_EVT) {
        if(p_data->connection_status.connected) {
            connection_id = p_data->connection_status.conn_id;
            notify_enabled = true;
        } else {
            connection_id = 0;
            notify_enabled = false;
            wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, BLE_ADDR_PUBLIC, NULL);
        }
    } else if (event == GATT_ATTRIBUTE_REQUEST_EVT) {
        if (p_data->attribute_request.opcode == GATT_REQ_WRITE || p_data->attribute_request.opcode == GATT_CMD_WRITE)
            return app_gatt_attr_write_handler(p_data->attribute_request.conn_id, p_data->attribute_request.opcode, &p_data->attribute_request.data.write_req);
        else if (p_data->attribute_request.opcode == GATT_REQ_READ)
            return app_gatt_attr_read_handler(p_data->attribute_request.conn_id, p_data->attribute_request.opcode, &p_data->attribute_request.data.read_req, p_data->attribute_request.len_requested);
    }
    return WICED_BT_GATT_SUCCESS;
}

static wiced_bt_gatt_status_t app_gatt_attr_read_handler(uint16_t conn_id, wiced_bt_gatt_opcode_t opcode, wiced_bt_gatt_read_t *p_read_req, uint16_t len_requested) {
    return wiced_bt_gatt_server_send_read_handle_rsp(conn_id, opcode, 0, NULL, NULL);
}