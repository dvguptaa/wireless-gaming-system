#include "main.h"
#include "ece453_pins.h"
#include "VL53L4CD_api.h"
#include "VL53L4CD_calibration.h"



#define SUB_ADDR  0x52
#define MEASUREMENT_THRESHOLD_MM 500
#define SDA_PIN P6_5
#define SCL_PIN P6_4
#define TOF_XSHUT P12_6
#define TOF_GPIO P12_7

#define IR_DETECTION_BIT (1 << 0)


extern VL53L4CD_ResultsData_t results;

// Command types for the IR task
typedef enum
{
    IR_COMMAND_INIT,
    IR_COMMAND_POLL_START,
    IR_COMMAND_POLL_STOP,
    IR_COMMAND_RESET
} ir_command_t;

void sensor_poll(void); 

// Message structure to send to the IR task
typedef struct
{
    ir_command_t command;
} ir_message_t;

// Public queue handle
extern QueueHandle_t q_ir;

// Public initialization function
cy_rslt_t task_ir_init(void);
