#include "light_sensor.h"

extern cyhal_i2c_t i2c_master_obj2;

uint8_t ltr_reg_read(uint8_t reg);

/**
 * @brief 
 *  Returns the value of the CONTR register
 * @return uint8_t 
 */
uint8_t ltr_light_get_contr(void)
{
    return ltr_reg_read(LTR_REG_CONTR);
}

/**
 * @brief 
 * returns the value of the STATUS register
 * @return uint8_t 
 */
uint8_t ltr_light_sensor_status(void)
{
    return ltr_reg_read(LTR_REG_ALS_STATUS);
}

/**
 * @brief
 * Returns the part ID of the LTR_329ALS-01
 * @return uint8_t
 */
uint8_t ltr_light_sensor_part_id(void)
{
    return ltr_reg_read(LTR_REG_PART_ID);
}

/**
 * @brief 
 *  Returns the manufacturer ID
 * @return uint8_t 
 */
uint8_t ltr_light_sensor_manufac_id(void)
{
    return ltr_reg_read(LTR_REG_MANUFAC_ID);
}


uint8_t ltr_reg_read(uint8_t reg)
{
    /* ADD CODE */
    cy_rslt_t rslt;
    uint8_t return_val;
    uint8_t write_data = reg;
    uint8_t read_data[2];
    rslt = cyhal_i2c_master_write(&i2c_master_obj2, LTR_SUBORDINATE_ADDR, &write_data, 1, 0, false);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    rslt = cyhal_i2c_master_read(&i2c_master_obj2, LTR_SUBORDINATE_ADDR, read_data, 1, 0 , true); 
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);


    return_val = read_data[0];
    return return_val;
}
/**
 * @brief 
 * Returns the Channel 0 data reading
 * @return uint16_t 
 */
uint16_t ltr_light_sensor_get_ch0(void)     //use read_reg method? 
{
    /* ADD CODE */
    uint8_t read_data[2];
    
    //Read CH0_0
    read_data[0] = ltr_reg_read(LTR_REG_ALS_DATA_CH0_0);
    //Read CH0_1
    read_data[1] = ltr_reg_read(LTR_REG_ALS_DATA_CH0_1);
    
    return ((read_data[1] << 8) | read_data[0]); 
  
}

/**
 * @brief 
 * Returns the Channel 1 data reading
 * @return uint16_t 
 */
uint16_t ltr_light_sensor_get_ch1(void)
{
    /* ADD CODE */
    uint8_t read_data[2];
    
    //Read CH0_0
    read_data[0] = ltr_reg_read(LTR_REG_ALS_DATA_CH1_0);
    //Read CH0_1
    read_data[1] = ltr_reg_read(LTR_REG_ALS_DATA_CH1_1);
    
    return ((read_data[1] << 8) | read_data[0]); 
  
}

/**
 * @brief
 * Set ALS MODE to Active and initiate a software reset
 */
void ltr_light_sensor_start(void)
{
    cy_rslt_t rslt;
    /* ADD CODE */
    uint8_t write_data[] = {LTR_REG_CONTR_ALS_MODE,LTR_REG_CONTR_SW_RESET};

    uint8_t write1[] = {LTR_REG_CONTR, write_data[0]};
    uint8_t write2[] = {LTR_REG_CONTR, write_data[1]};

    rslt = cyhal_i2c_master_write(&i2c_master_obj2, LTR_SUBORDINATE_ADDR, write2, 2, 0 ,true);  
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    rslt = cyhal_i2c_master_write(&i2c_master_obj2, LTR_SUBORDINATE_ADDR, write1, 2, 0 ,true);  
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
}

cy_rslt_t light_sensor_init() {
    uint8_t manufac_id = 0x05;
    uint8_t part_id = 0xA0;
    if (ltr_light_sensor_manufac_id() != manufac_id) {
        return CY_RSLT_TYPE_ERROR;
    }
    if (ltr_light_sensor_part_id() != part_id) {
        return CY_RSLT_TYPE_ERROR;
    }


    ltr_light_sensor_start();
    return CY_RSLT_SUCCESS;
}



