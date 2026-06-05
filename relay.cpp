#include "relay.h"

// 灯
int relay1_pin = 6;

// 风扇
int relay2_pin = 7;

void relay_init()
{
    pinMode(relay1_pin, OUTPUT);

    pinMode(relay2_pin, OUTPUT);

    digitalWrite(relay1_pin, LOW);

    digitalWrite(relay2_pin, LOW);
}

void relay_update(bool master_state,
                  uint8_t mode)
{
    // KEY1关闭
    if(!master_state)
    {
        digitalWrite(relay1_pin, LOW);

        digitalWrite(relay2_pin, LOW);

        return;
    }

    switch(mode)
    {
        // 00
        case 0:
            digitalWrite(relay1_pin, LOW);
            digitalWrite(relay2_pin, LOW);
            break;

        // 01
        case 1:
            digitalWrite(relay1_pin, LOW);
            digitalWrite(relay2_pin, HIGH);
            break;

        // 10
        case 2:
            digitalWrite(relay1_pin, HIGH);
            digitalWrite(relay2_pin, LOW);
            break;

        // 11
        case 3:
            digitalWrite(relay1_pin, HIGH);
            digitalWrite(relay2_pin, HIGH);
            break;
    }
}
void relay_all_off()
{
    digitalWrite(relay1_pin, LOW);

    digitalWrite(relay2_pin, LOW);
}
void Light_On()
{
    digitalWrite(relay1_pin, HIGH);
}

void Light_Off()
{
    digitalWrite(relay1_pin, LOW);
}