#ifndef __EXTI_H__
#define __EXTI_H__

#include <Arduino.h>

extern volatile uint8_t relay_mode;

extern volatile bool auto_mode;

extern volatile bool mode_changed;

void exti_init();

void key2_scan();

#endif