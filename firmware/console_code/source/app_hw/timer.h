#include "main.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "event_groups.h"


extern cyhal_timer_t timer_obj;
extern EventGroupHandle_t timer_event;

#define LS_EVENT_BIT (1<<0)
#define TOF_EVENT_BIT (1 << 1)
#define ALL_TIMER_BITS (LS_EVENT_BIT | TOF_EVENT_BIT)


void timer_init(void);
void timer_isr(void *callback_arg, cyhal_timer_event_t event);
void timer_start(void);
void timer_stop(void);