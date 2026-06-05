#ifndef __RGB_LED_H__
#define __RGB_LED_H__

#include <Arduino.h>

void rgb_led_init();
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);
void rgb_led_update(bool wifi_ok, bool mqtt_ok, bool light_on, bool fan_on);

#endif
