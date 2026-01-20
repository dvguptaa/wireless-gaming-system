#ifndef __LIGHT_SENSOR_H__
#define __LIGHT_SENSOR_H__

#include "main.h"

#define LTR_SUBORDINATE_ADDR    0x29

#define LTR_REG_CONTR           0x80
#define LTR_REG_MEAS_RATE       0x85
#define LTR_REG_PART_ID         0x86
#define LTR_REG_MANUFAC_ID      0x87
#define LTR_REG_ALS_DATA_CH1_0  0x88    //CH1 Lower byte
#define LTR_REG_ALS_DATA_CH1_1  0x89    //CH1 Upper byte
#define LTR_REG_ALS_DATA_CH0_0  0x8A    //CH0 Lower byte
#define LTR_REG_ALS_DATA_CH0_1  0x8B    //CH0 Upper Byte
#define LTR_REG_ALS_STATUS      0x8C

#define LTR_REG_CONTR_SW_RESET  (1 << 1)
#define LTR_REG_CONTR_ALS_MODE  (1 << 0)

#define LTR_REG_STATUS_NEW_DATA     (1 << 2)
#define LTR_REG_STATUS_VALID_DATA   (1 << 7)



void ltr_light_sensor_start(void);
uint8_t ltr_light_sensor_part_id(void);
uint8_t ltr_light_sensor_manufac_id(void);
uint16_t ltr_light_sensor_get_ch0(void);
uint16_t ltr_light_sensor_get_ch1(void);
uint8_t ltr_light_status(void);
uint8_t ltr_light_get_contr(void);
cy_rslt_t light_sensor_init();






#endif