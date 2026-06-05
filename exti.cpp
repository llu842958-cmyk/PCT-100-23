#include "exti.h"
#include "key.h"
#include "temp.h"
#include "light.h"
#include "relay.h"

// 默认AUTO模式
volatile bool auto_mode = true;

volatile bool mode_changed = false;

// 四状态
volatile uint8_t relay_mode = 0;

// 按键状态
bool last_key_state = LOW;

unsigned long press_time = 0;

// 防止长按连续触发
bool long_press_triggered = false;

void exti_init()
{
    // 不使用中断
}

void key2_scan()
{
    bool current_state = digitalRead(key2_pin);

    if(current_state == HIGH && last_key_state == LOW)
    {
        press_time = millis();
        long_press_triggered = false;
    }

    if(current_state == HIGH)
    {
        unsigned long hold_time = millis() - press_time;

        if(hold_time >= 2000 && !long_press_triggered)
        {
            long_press_triggered = true;
            auto_mode = !auto_mode;

            if(auto_mode)
            {
                relay_mode = 0;
                temp_reset();
                relay_all_off();
            }

            mode_changed = true;
        }
    }

    if(current_state == LOW && last_key_state == HIGH)
    {
        unsigned long hold_time = millis() - press_time;

        if(hold_time > 50 && hold_time < 800)
        {
            if(!auto_mode)
            {
                relay_mode++;
                if(relay_mode > 3)
                {
                    relay_mode = 0;
                }
            }
        }
    }

    last_key_state = current_state;
}
