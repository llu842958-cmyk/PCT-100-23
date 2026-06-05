#include "display.h"
#include "exti.h"
#include "temp.h"
#include "light.h"
#include "relay.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 5, 4);

void display_init() { u8g2.begin(); u8g2.enableUTF8Print(); }

void display_update(bool key1_state, bool wifi_conn, String wifi_ip, String mqtt_ip, bool mqtt_conn)
{
    bool lo = digitalRead(relay1_pin);
    bool fo = digitalRead(relay2_pin);
    unsigned long lux = (4095 - light_adc); lux = (lux*lux)/30000;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);

    u8g2.setCursor(2, 11);  u8g2.printf("模式:%s",   auto_mode?"自动":"手动");
    u8g2.setCursor(72, 11); u8g2.printf("总闸:%s",   key1_state?"ON":"OFF");

    u8g2.setCursor(2, 21);  u8g2.printf("光照:%d/%d", (unsigned int)lux, light_lux_th);

    u8g2.setCursor(2, 31);  u8g2.printf("温度:%.1f/%.1f", current_temp, fan_on_temp);

    u8g2.setCursor(2, 41);  u8g2.printf("灯光:%s",   lo?"ON":"OFF");
    u8g2.setCursor(72, 41); u8g2.printf("风扇:%s",   fo?"ON":"OFF");

    u8g2.setCursor(2, 51);  u8g2.printf("WiFi:%s",   wifi_conn?"已连接":"未连接");

    // MQTT: 用小字体显示完整 IP
    u8g2.setCursor(2, 61);
    if (mqtt_conn && mqtt_ip.length() > 0) {
        u8g2.setFont(u8g2_font_5x8_tf);
        u8g2.print(mqtt_ip.c_str());
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    } else {
        u8g2.print("MQTT:OFF");
    }

    u8g2.sendBuffer();
}
