#ifndef __TASK_IMU_H__
#define __TASK_IMU_H__

#include "main.h"

/* Gesture Definitions - Explicit Values */
typedef enum {
    GESTURE_NONE  = 0x00,
    GESTURE_LEFT  = 0x03,  // Was 3, now 01
    GESTURE_RIGHT = 0x04,  // Was 4, now 02
    GESTURE_UP    = 0x01,  // Was 1, now 03
    GESTURE_DOWN  = 0x02   // Was 2, now 04
} imu_gesture_t;

/* Data Structure for Bluetooth */
typedef struct {
    int16_t heading;
    int16_t roll;
    int16_t pitch;
    imu_gesture_t gesture;
} imu_data_t;

/* Calibration Structure to save in EEPROM */
typedef struct {
    int16_t center_roll;
    int16_t center_pitch;
    uint8_t magic_num; 
} imu_calib_t;

void task_imu_get_data(imu_data_t *data);
bool task_imu_resources_init(void);
void task_imu(void *arg);

/* Function to trigger calibration from Bluetooth Task */
void task_imu_req_calibration(void);

#endif /* __TASK_IMU_H__ */