#include "console.h"
#include "task_ble.h"

//Global vars
int16_t speaker_buffer[256];
int wall_hit = 0;
int dark_flag = 0;
extern volatile uint8_t ble_motor_request;
int TOF_activate = 1;


static uint8_t current_high_score = 0;

int console_init() {

    BaseType_t task_status;
    cy_rslt_t result;

    result = i2c_init(MODULE_SITE_2);
    if (result != CY_RSLT_SUCCESS)
    {
        return -1;  //I2C not init
    }

    result = task_uart_init();
    if (result != CY_RSLT_SUCCESS) {
        return -2;
    }

    uart_register_rx_callback(uart_score_callback);

    timer_init();
    
    result = init_EEPROM();
    if (result != CY_RSLT_SUCCESS) {
        return -3;
    }


    result = light_sensor_init();
    if (result != CY_RSLT_SUCCESS) {
        return -4;
    }


    result = task_ir_init();
    if (result != CY_RSLT_SUCCESS) {
        return -5;
    }

    result = speakers_init();
    if (result != CY_RSLT_SUCCESS) {
        return -6;
    }


    task_status = xTaskCreate(
      LR_task,
      "Light sensor task",
      512,
      NULL,
      configMAX_PRIORITIES - 6,
      NULL);

    if (task_status != pdPASS) {
        return -7;
    } 


    task_status = xTaskCreate(
      TOF_task,
      "TOF task",
      256,
      NULL,
      configMAX_PRIORITIES - 4,
      NULL);

    if (task_status != pdPASS) {
        return -8;
      } 

    task_status = xTaskCreate(
      EEPROM_task,
      "EEPROM Task",
      256,
      NULL,
      configMAX_PRIORITIES - 6,
      NULL);

    if (task_status != pdPASS) {
        return -9;
    }  


    task_status = xTaskCreate(
      Speaker_task,
      "Speaker task",
      256,
      NULL,
      configMAX_PRIORITIES - 6,
      NULL);

    if (task_status != pdPASS) {
        return -10;
      } 

    
    // printf("All initialized\r\n");
    return 0;

}

    

void uart_score_callback(uint8_t *data, uint16_t length)
{

    // Check if received data is "MENU"
    if (length >= 4 && strncmp((char *)data, "MENU", 4) == 0)
    {
        // Read high score from EEPROM
        current_high_score = eeprom_read(0x01);

         // Send high score to Pi
        char msg[16];
        snprintf(msg, sizeof(msg), "S %d\n", current_high_score);
        uart_send_string(msg);
        
        TOF_activate = 1;
            

    }
    else if (length >= 6 && strncmp((char *)data, "RUMBLE", 6) == 0)
    {
        wall_hit = 1;
        xEventGroupSetBits(wall_event, WALL_EVENT_BIT);
    }

    else if (strncmp((char*)data, "WIN ", 4) == 0)
    {
        if (wall_event != NULL) {
        xEventGroupSetBits(wall_event, VICTORY_EVENT_BIT);
        }

        int time = atof((char*)data + 4);

        if (time >= 254) {
            eeprom_write(0xFE, 0x01);
        }

        else if (time < current_high_score)
        {
            current_high_score = time;
            eeprom_write((uint8_t)time, 0x01);
        }
        
    }
    else if (strncmp((char*)data, "VIC", 3) == 0)
    {
        if (wall_event != NULL) {
        xEventGroupSetBits(wall_event, VICTORY_EVENT_BIT);
        }

        
        
    }
    else if (length >= 5 && strncmp((char *)data, "LEVEL", 5) == 0)
    {
                TOF_activate = 1;
        
    }

}

void EEPROM_task(void *param)
{

    (void)param;
    vTaskDelay(pdMS_TO_TICKS(200));

    current_high_score = eeprom_read(0x01);
    // printf("eeprom read\r\n");
    
 
    for (;;)    
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void LR_task(void *param) {

    (void)param;

    char msg[16];
    vTaskDelay(pdMS_TO_TICKS(3000));
    

    while(1) {
    EventBits_t bits = xEventGroupWaitBits(
        timer_event,               // Event group handle
        LS_EVENT_BIT,          // Bit to wait for
        pdTRUE,                   // Clear bit on exit (auto-reset)
        pdFALSE,                  // Wait for ANY bit (only one bit here)
        portMAX_DELAY             // Wait forever
    );

    if (bits & LS_EVENT_BIT) {
        uint16_t light_data = ltr_light_sensor_get_ch0();
        if (light_data < 1) {
            dark_flag = 1; 
            snprintf(msg, sizeof(msg), "dark 1\n");
            uart_send_string(msg);
        }
        else {
            dark_flag = 0;
            snprintf(msg, sizeof(msg), "dark 0\n");
            uart_send_string(msg);
        }
    }
}
}


void TOF_task(void *param) {

    (void)param;
    vTaskDelay(pdMS_TO_TICKS(3000));
    

    while(1) {
    EventBits_t bits = xEventGroupWaitBits(
        timer_event,               // Event group handle
        TOF_EVENT_BIT | VICTORY_EVENT_BIT,          // Bit to wait for
        pdTRUE,                   // Clear bit on exit (auto-reset)
        pdFALSE,                  // Wait for ANY bit (only one bit here)
        portMAX_DELAY             // Wait forever
    );

    if (bits & TOF_EVENT_BIT && TOF_activate) { //do not activate TOF if victory sound is playing
        sensor_poll();
    }
    }
}



// void Speaker_task(void* pvParameters) {   //When called play sound
//     speaker_volume(speaker_buffer, 256, VOLUME_PERCENT);

//     while(1) {

//         EventBits_t bits = xEventGroupWaitBits(
//         wall_event,               // Event group handle
//         WALL_EVENT_BIT,          // Bit to wait for
//         pdTRUE,                   // Clear bit on exit (auto-reset)
//         pdFALSE,                  // Wait for ANY bit (only one bit here)
//         portMAX_DELAY             // Wait forever
//         );

//         if (bits & WALL_EVENT_BIT) { 
//             task_ble_send_motor_cmd(ble_motor_request);
//             wall_hit_note(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ, 880.0);      //32000
//             // wall_hit_note(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ, 770.01);  //16800
             
//         }
//         // Handle Victory
//         if (bits & VICTORY_EVENT_BIT) {
//             speaker_mario_victory(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
//         }
//     }
        
// }

void Speaker_task(void* pvParameters) {
    speaker_volume(speaker_buffer, 256, VOLUME_PERCENT);

    while(1) {
        // CHANGE THIS LINE BELOW:
        // We act as a listener for EITHER a wall hit OR a victory
        EventBits_t bits = xEventGroupWaitBits(
            wall_event,                        
            WALL_EVENT_BIT | VICTORY_EVENT_BIT | CONNECTION_EVENT_BIT, // <--- YOU MUST ADD THIS PART
            pdTRUE,                   
            pdFALSE,                  
            portMAX_DELAY             
        );

        if (bits & WALL_EVENT_BIT) { 
            task_ble_send_motor_cmd(ble_motor_request);
            // speaker_wall_bump(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ); // Use your preferred sound here
             wall_hit_note(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ, 880.0);
            // speaker_mario(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
        }

        // ADD THIS CHECK:
        if (bits & VICTORY_EVENT_BIT) {
            TOF_activate = 0; //disable TOF during victory sound
            speaker_mario_victory(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
        }

        if(bits & CONNECTION_EVENT_BIT) {
            // Play a sound on connection
            speaker_mario_coin(speaker_buffer, 256, AUDIO_SAMPLE_RATE_HZ);
        }
    }
}