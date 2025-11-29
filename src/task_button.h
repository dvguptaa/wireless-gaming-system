#ifndef TASK_BUTTON_H
#define TASK_BUTTON_H

#include <stdbool.h>

/* Global Shared Variable (Read Only for other tasks) */
extern volatile bool g_btn_pressed;

/* Initialization Function */
void task_button_init(void);

#endif