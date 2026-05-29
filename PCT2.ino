#include "version.h"
#include "key.h"
#include "relay.h"
#include "exti.h"

void setup()
{
    Serial.begin(115200);

    delay(1000);

    key_init();

    relay_init();

    exti_init();

    Serial.println("System Start");

    Serial.println("AUTO MODE");
}

void loop()
{
    // KEY2扫描
    key2_scan();

    // KEY1总开关
    bool key1_state =
    digitalRead(key1_pin);

    // KEY1关闭
    if(!key1_state)
    {
        // 回到00
        relay_mode = 0;

        // 回到AUTO模式
        auto_mode = true;
    }

    // 更新继电器
    relay_update(
        key1_state,
        relay_mode
    );

    // ===== 模式打印 =====
    static bool last_auto_mode =
    auto_mode;

    if(last_auto_mode != auto_mode)
    {
        last_auto_mode = auto_mode;

        if(auto_mode)
        {
            Serial.println(
            "AUTO MODE");
        }
        else
        {
            Serial.println(
            "MANUAL MODE");
        }
    }

    delay(10);
}