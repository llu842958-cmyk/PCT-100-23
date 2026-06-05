#include "rgb_led.h"
#include <Adafruit_NeoPixel.h>

#define PIN   0
#define NUMS  1

static Adafruit_NeoPixel strip(NUMS, PIN, NEO_GRB + NEO_KHZ800);

enum LedMode {
    LM_BOOT, LM_CRITICAL, LM_LIGHT_ALARM, LM_TEMP_ALARM,
    LM_MQTT_DOWN, LM_WIFI_DOWN, LM_NORMAL
};
static LedMode mode = LM_BOOT;
static unsigned long t0 = 0;
static int  step = 0;

void rgb_led_init() {
    strip.begin(); strip.setBrightness(32); strip.clear(); strip.show();
    mode=LM_BOOT; step=0; t0=millis();
}
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(0, strip.Color(r,g,b)); strip.show();
}

void rgb_led_update(bool wifi_ok, bool mqtt_ok, bool light_on, bool fan_on) {
    unsigned long now = millis();

    // ===== 开机 =====
    if (mode == LM_BOOT) {
        static int bc = 0, ph = 0;
        if (now - t0 > 200) { t0=now; ph=(ph+1)%3; if(ph==0) bc++;
            if (bc>=2) { mode=LM_NORMAL; strip.clear(); strip.show(); return; }
            if(ph==0)rgb_led_set(255,0,0); else if(ph==1)rgb_led_set(0,255,0); else rgb_led_set(0,0,255);
        }
        return;
    }

    // ===== 优先级判断 =====
    LedMode nm;
    if (light_on && fan_on)           nm = LM_CRITICAL;
    else if (light_on && !fan_on)     nm = LM_LIGHT_ALARM;
    else if (!light_on && fan_on)     nm = LM_TEMP_ALARM;
    else if (!mqtt_ok)                nm = LM_MQTT_DOWN;
    else if (!wifi_ok)                nm = LM_WIFI_DOWN;
    else                              nm = LM_NORMAL;
    if (nm != mode) { mode = nm; step = 0; t0 = now; }

    // ===== 1. 严重警报 R×2 G×2 B×2 =====
    if (mode == LM_CRITICAL) {
        const uint8_t c[][3] = {{255,0,0},{0,0,0}, {255,0,0},{0,0,0}, {0,255,0},{0,0,0}, {0,255,0},{0,0,0}, {0,0,255},{0,0,0}, {0,0,255},{0,0,0}};
        const unsigned long d[] = {100,50, 100,50, 100,50, 100,50, 100,50, 100,350};
        const int N = 12;
        if (now-t0 > d[step]) { t0=now; step=(step+1)%N; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }

    // ===== 2. 光照越界 R300 G300 B300 =====
    if (mode == LM_LIGHT_ALARM) {
        const uint8_t c[][3] = {{255,0,0},{0,0,0}, {0,255,0},{0,0,0}, {0,0,255},{0,0,0}};
        const unsigned long d[] = {300,200, 300,200, 300,200};
        const int N = 6;
        if (now-t0 > d[step]) { t0=now; step=(step+1)%N; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }

    // ===== 3. 温度越界 R500 G500 B500 灭700 =====
    if (mode == LM_TEMP_ALARM) {
        const uint8_t c[][3] = {{255,0,0},{0,255,0},{0,0,255},{0,0,0}};
        const unsigned long d[] = {500,500,500,700};
        const int N = 4;
        if (now-t0 > d[step]) { t0=now; step=(step+1)%N; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }

    // ===== 4. MQTT 未连 红→绿→灭→蓝→灭 各500ms =====
    if (mode == LM_MQTT_DOWN) {
        const uint8_t c[][3] = {{255,0,0},{0,255,0},{0,0,0},{0,0,255},{0,0,0}};
        const int N = 5;
        if (now-t0 > 500) { t0=now; step=(step+1)%N; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }

    // ===== 5. WiFi 未连 R200 G200 灭500 =====
    if (mode == LM_WIFI_DOWN) {
        const uint8_t c[][3] = {{255,0,0},{0,255,0},{0,0,0}};
        const unsigned long d[] = {200,200,500};
        const int N = 3;
        if (now-t0 > d[step]) { t0=now; step=(step+1)%N; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }

    // ===== 6. 正常 灭→红1s→绿1s→蓝1s→灭1s→灭200ms =====
    if (mode == LM_NORMAL) {
        const uint8_t c[][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{0,0,0},{0,0,0}};
        const unsigned long d[] = {0,1000,1000,1000,1000,200};
        const int N = 6;
        if (now-t0 > d[step]) { t0=now; step=(step+1)%N; if(step==0) step=1; }
        rgb_led_set(c[step][0],c[step][1],c[step][2]);
        return;
    }
}
