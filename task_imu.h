#ifndef TASK_IMU_H
#define TASK_IMU_H

#include "main.h"
#include <stdbool.h>

/* Data Structure for IMU angles */
typedef struct {
    float heading;
    float roll;
    float pitch;
} fusion_imu_t;

/* Global Shared Variable (Read Only for other tasks) */
extern volatile fusion_imu_t g_imu_data;

/* Initialization Function */
bool task_imu_resources_init(void);

/* Task Entry Point */
void task_imu(void *arg);

#endif