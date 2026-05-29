#include "exti.h"
#include "key.h"

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
    bool current_state =
    digitalRead(key2_pin);

    // ===== 按下瞬间 =====
    if(current_state == HIGH &&
       last_key_state == LOW)
    {
        press_time = millis();

        long_press_triggered = false;
    }

    // ===== 按住过程中 =====
    if(current_state == HIGH)
    {
        unsigned long hold_time =
        millis() - press_time;

        // ===== 长按2秒 =====
        if(hold_time >= 2000 &&
           !long_press_triggered)
        {
            long_press_triggered = true;

            // 切换模式
            auto_mode = !auto_mode;

            // 切换到AUTO
            if(auto_mode)
            {
                // 强制恢复00
                relay_mode = 0;
            }

            mode_changed = true;
        }
    }

    // ===== 松开瞬间 =====
    if(current_state == LOW &&
       last_key_state == HIGH)
    {
        unsigned long hold_time =
        millis() - press_time;

        // ===== 短按 =====
        if(hold_time > 50 &&
           hold_time < 500)
        {
            // 只有手动模式有效
            if(!auto_mode)
            {
                relay_mode++;

                if(relay_mode > 3)
                {
                    relay_mode = 0;
                }

                Serial.print(
                "Relay Mode = ");

                Serial.println(
                relay_mode);
            }
        }
    }

    last_key_state = current_state;
}