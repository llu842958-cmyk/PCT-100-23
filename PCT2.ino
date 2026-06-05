#include "version.h"
#include "key.h"
#include "relay.h"
#include "exti.h"
#include "light.h"
#include "temp.h"
#include "display.h"
#include "wifi_manager.h"
#include "rgb_led.h"
#include "mqtt_manager.h"

void setup()
{
    Serial.begin(115200);
    Serial.println("系统启动");

    key_init();
    relay_init();
    exti_init();
    display_init();
    light_init();
    temp_init();
    rgb_led_init();
    wifi_init();
    mqtt_init();

    rgb_led_update(wifi_connected, mqtt_connected, false, false);
}

void loop()
{
    wifi_loop();
    mqtt_loop();

    // 统一串口处理 — 一行只读一次，根据内容分发
    if (Serial.available()) {
        String s = Serial.readStringUntil('\n');
        s.trim();
        if (s.length() > 0) {
            if (s.startsWith("mqtt")) {
                mqtt_handle_command(s);
            } else {
                wifi_handle_command(s);
            }
        }
    }

    // LED 状态 — 传入完整参数
    static unsigned long led_timer = 0;
    if (millis() - led_timer > 50) {
        led_timer = millis();
        bool lo = digitalRead(relay1_pin);
        bool fo = digitalRead(relay2_pin);
        rgb_led_update(wifi_connected, mqtt_connected, lo, fo);
    }

    key2_scan();
    bool k1 = digitalRead(key1_pin);

    if (!k1) {
        relay_mode = 0;
        auto_mode  = true;
        temp_reset();
        relay_all_off();
    } else {
        if (auto_mode) {
            light_update();
            temp_update();
        } else {
            relay_update(k1, relay_mode);
        }
    }

    static bool last_auto = auto_mode;
    if (last_auto != auto_mode) {
        last_auto = auto_mode;
        Serial.print("[模式] ");
        Serial.println(auto_mode ? "自动" : "手动");
    }

    static unsigned long ts = 0;
    if (millis() - ts > 1000) {
        ts = millis();
        display_update(k1, wifi_connected, wifi_ip, mqtt_ip_global, mqtt_connected);
    }

    delay(10);
}
