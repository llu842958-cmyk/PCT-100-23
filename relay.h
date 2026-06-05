#ifndef __RELAY_H__
#define __RELAY_H__

#include <Arduino.h>

extern int relay1_pin;
extern int relay2_pin;

void relay_init();

void relay_update(bool master_state,
                  uint8_t mode);

// 新增这两个声明
void Light_On();

void Light_Off();
void relay_all_off();
#endif