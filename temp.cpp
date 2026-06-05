#include "temp.h"
#include "relay.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMP_PIN 10

float fan_on_temp  = 31.0;
float fan_off_temp = 30.0;
float current_temp = 0;
bool  fan_state    = false;

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

void temp_init()
{
    sensors.begin();
}

void temp_update()
{
    static unsigned long lastRead = 0;
    if(millis() - lastRead < 500) return;
    lastRead = millis();

    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    static float last_temp = -100;
    if(abs(temp - last_temp) > 0.1) {
        current_temp = temp;
        last_temp = temp;
    }

    if(!fan_state && temp > fan_on_temp) {
        fan_state = true;
        digitalWrite(relay2_pin, HIGH);
    }

    if(fan_state && temp < fan_off_temp) {
        fan_state = false;
        digitalWrite(relay2_pin, LOW);
    }
}

void temp_reset()
{
    fan_state = false;
}
