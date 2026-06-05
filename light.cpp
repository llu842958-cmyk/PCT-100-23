#include "light.h"
#include "exti.h"
#include "relay.h"

int   light_on_th  = 1500;
int   light_off_th = 1000;
int   light_lux_th = 200;   // lux 阈值（MQTT/显示用）
int   light_adc    = 0;
float light_voltage = 0;

void light_init()
{
    analogReadResolution(12);
}

void light_update()
{
    int adcValue = 0;
    for(int i = 0; i < 5; i++)
        adcValue += analogRead(ADC_PIN);
    adcValue /= 5;
    light_adc = adcValue;

    light_voltage = adcValue * (3.3 / 4095.0);

    static bool light_state = false;

    if(!light_state && adcValue > light_on_th)
    {
        light_state = true;
        Light_On();
    }

    if(light_state && adcValue < light_off_th)
    {
        light_state = false;
        Light_Off();
    }
}
