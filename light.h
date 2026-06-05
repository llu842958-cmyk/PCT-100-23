#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <Arduino.h>

#define ADC_PIN 1

extern int   light_on_th;
extern int   light_off_th;
extern int   light_lux_th;   // MQTT 直读 lux 阈值
extern int   light_adc;
extern float light_voltage;

void light_init();
void light_update();

#endif
