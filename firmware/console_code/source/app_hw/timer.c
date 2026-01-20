#include "timer.h"


cyhal_timer_t timer_obj;
EventGroupHandle_t timer_event = NULL;



void timer_isr(void *callback_arg, cyhal_timer_event_t event) {
    (void)callback_arg;
    (void)event;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xEventGroupSetBitsFromISR(timer_event, ALL_TIMER_BITS, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}


void timer_init(void) {
    cy_rslt_t rslt;
    cyhal_timer_cfg_t timer_cfg;

    timer_event = xEventGroupCreate();
    if (timer_event == NULL) {
        CY_ASSERT(0); // Event group creation failed
    }

    uint32_t ticks = 50000000; //2 seconds
    
    timer_cfg.compare_value = 0;
    timer_cfg.period = ticks - 1;
    timer_cfg.direction = CYHAL_TIMER_DIR_UP;
    timer_cfg.is_compare = false;
    timer_cfg.is_continuous = true;
    timer_cfg.value = 0;
    
    // Initialize timer
    rslt = cyhal_timer_init(&timer_obj, NC, NULL);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
    
    // Configure timer
    rslt = cyhal_timer_configure(&timer_obj, &timer_cfg);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
    
    // Set frequency to 100 MHz
    rslt = cyhal_timer_set_frequency(&timer_obj, 100000000);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
    
    // Register ISR callback
    cyhal_timer_register_callback(&timer_obj, timer_isr, NULL);
    
    // Enable terminal count interrupt with priority 3
    cyhal_timer_enable_event(&timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 7, true);
    
    // Start the timer
    rslt = cyhal_timer_start(&timer_obj);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
}

void timer_start(void)
{
    cy_rslt_t rslt = cyhal_timer_start(&timer_obj);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
}

void timer_stop(void)
{
    cy_rslt_t rslt = cyhal_timer_stop(&timer_obj);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS);
}





