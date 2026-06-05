#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "temp.h"
#include "light.h"
#include "exti.h"
#include "key.h"
#include "relay.h"
#include <Preferences.h>

// ====== 默认配置 ======
static String mqtt_ip   = "47.98.170.180";
static int    mqtt_port = 8081;
static String mqtt_user = "dzdx_emqx";
static String mqtt_pass = "Jp4!sQ7$";

String  mqtt_ip_global = "47.98.170.180";  // 暴露给外部
static String device_id = "PCT_100_23A";
static bool     id_need_reconnect = false;

bool mqtt_connected = false;

static WiFiClient   wifi_client;
static PubSubClient mqtt(wifi_client);

static unsigned long last_upload = 0;
static unsigned long last_recon  = 0;
static bool          need_upload = false;

#define UPLOAD_INTERVAL 3000   // 3秒上报
#define RECON_INTERVAL  10000

// ====== Flash ======
static void mqtt_config_save()
{
    Preferences p;
    p.begin("mqtt", false);
    p.putString("ip",   mqtt_ip);
    p.putInt   ("port", mqtt_port);
    p.putString("user", mqtt_user);
    p.putString("pass", mqtt_pass);
    p.putString("id",   device_id);
    p.end();
    Serial.printf("[MQTT] 保存到Flash id=%s\n", device_id.c_str());
}

static void mqtt_config_load()
{
    Preferences p;
    p.begin("mqtt", true);
    mqtt_ip   = p.getString("ip",   "47.98.170.180");
    mqtt_port = p.getInt   ("port", 8081);
    mqtt_user = p.getString("user", "dzdx_emqx");
    mqtt_pass = p.getString("pass", "Jp4!sQ7$");
    device_id = p.getString("id",   "PCT_100_23A");
    mqtt_ip_global = mqtt_ip;  // 同步全局变量
    p.end();
    Serial.printf("[MQTT] load flash id=%s\n", device_id.c_str());
}

// ====== 阈值保存 ======
static void threshold_save()
{
    Preferences p;
    p.begin("thr", false);
    p.putFloat("fan_on",  fan_on_temp);
    p.putFloat("fan_off", fan_off_temp);
    p.putInt  ("lt_on",   light_on_th);
    p.putInt  ("lt_off",  light_off_th);
    p.putInt  ("lux_th",  light_lux_th);
    p.end();
}

static void threshold_load()
{
    Preferences p;
    p.begin("thr", true);
    fan_on_temp  = p.getFloat("fan_on",  31.0);
    fan_off_temp = p.getFloat("fan_off", 30.0);
    light_on_th  = p.getInt  ("lt_on",   1500);
    light_off_th = p.getInt  ("lt_off",  1000);
    light_lux_th = p.getInt  ("lux_th",  200);
    p.end();
}

// ====== MQTT 回调 ======
static void mqtt_callback(char* topic, byte* payload, unsigned int len)
{
    // 空消息跳过
    if (len == 0) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, len);
    if (err) {
        Serial.print("[MQTT] JSON解析失败: ");
        Serial.println(err.c_str());
        return;
    }

    String cmd = doc["cmd"] | "";

    // 没有 cmd 字段 = 自己的 status 回显，忽略
    if (cmd.length() == 0) return;

    if (cmd != "get_status") {
        Serial.printf("[MQTT] ↓ %s: ", topic);
        Serial.write(payload, len);
        Serial.println();
    }

    // ---- set_relay ----
    if (cmd == "set_relay") {
        int  r = doc["relay"] | 0;
        bool v = doc["value"] | false;
        bool k1 = digitalRead(key1_pin);

        if (!k1) {
            Serial.println("[MQTT] KEY1 关闭，忽略 set_relay");
            return;
        }

        if (r == 3) {
            digitalWrite(relay1_pin, v ? HIGH : LOW);
            Serial.printf("[MQTT] 灯光 → %s\n", v ? "ON" : "OFF");
        }
        else if (r == 4) {
            digitalWrite(relay2_pin, v ? HIGH : LOW);
            Serial.printf("[MQTT] 风机 → %s\n", v ? "ON" : "OFF");
        }

        // 同步 relay_mode，防止 relay_update() 覆盖
        bool l = digitalRead(relay1_pin);
        bool f = digitalRead(relay2_pin);
        relay_mode = (l ? 2 : 0) | (f ? 1 : 0);
        need_upload = true;  // 立即上报新状态
    }
    // ---- set_mode ----
    else if (cmd == "set_mode") {
        String mode = doc["mode"] | "";
        if (mode == "auto") {
            auto_mode = true;
            relay_mode = 0;
            temp_reset();
            relay_all_off();
            Serial.println("[MQTT] 切换 → 自动模式");
        }
        else if (mode == "manual") {
            auto_mode = false;
            Serial.println("[MQTT] 切换 → 手动模式");
        }
        need_upload = true;
    }
    // ---- get_status ----
    else if (cmd == "get_status") {
        need_upload = true;
    }
    // ---- set_threshold ----
    else if (cmd == "set_threshold") {
        bool changed = false;
        // 兼容多种字段名
        float t = 0;
        if (doc.containsKey("temp")) t = doc["temp"];
        else if (doc.containsKey("temperature")) t = doc["temperature"];
        else if (doc.containsKey("temp_threshold")) t = doc["temp_threshold"];
        if (t > 0) {
            fan_on_temp  = t;
            fan_off_temp = t - 1.0f;
            if (fan_off_temp < 0) fan_off_temp = 0;
            Serial.printf("[MQTT] 温度阈值 → %.1f ℃\n", t);
            changed = true;
        }
        int l = 0;
        if (doc.containsKey("light")) l = doc["light"];
        else if (doc.containsKey("light_threshold")) l = doc["light_threshold"];
        else if (doc.containsKey("lux")) l = doc["lux"];
        if (l > 0) {
            light_lux_th = l;  // 直接存 lux
            int adc_th = 4095 - (int)sqrt((float)l * 30000.0f);
            if (adc_th < 0) adc_th = 0;
            light_on_th  = adc_th;
            light_off_th = adc_th * 2 / 3;
            Serial.printf("[MQTT] 光照阈值 → %d lux (ADC:%d)\n", l, adc_th);
            changed = true;
        }
        if (changed) { threshold_save(); need_upload = true; }
    }
    // ---- reboot ----
    else if (cmd == "reboot") {
        Serial.println("[MQTT] 远程重启...");
        delay(100);
        ESP.restart();
    }
}

// ====== 连接 ======
static bool mqtt_do_connect()
{
    if (!wifi_connected) {
        Serial.println("[MQTT] WiFi未连接，跳过");
        return false;
    }

    mqtt.setServer(mqtt_ip.c_str(), mqtt_port);
    mqtt.setBufferSize(512);
    mqtt.setCallback(mqtt_callback);
    mqtt.setKeepAlive(30);

    String client_id = device_id + "_dev";

    Serial.printf("[MQTT] 连接 %s:%d (client=%s)... ", mqtt_ip.c_str(), mqtt_port, client_id.c_str());

    if (mqtt.connect(client_id.c_str(), mqtt_user.c_str(), mqtt_pass.c_str())) {
        String topic_cmd = "chemctrl/" + device_id + "/command";
        String topic_sta = "chemctrl/" + device_id + "/status";
        mqtt.subscribe(topic_cmd.c_str());
        mqtt.subscribe(topic_sta.c_str());
        mqtt_connected = true;
        Serial.printf("OK (订阅:%s + %s)\n", topic_cmd.c_str(), topic_sta.c_str());
        need_upload = true;
        return true;
    } else {
        Serial.printf("FAIL rc=%d\n", mqtt.state());
        mqtt_connected = false;
        return false;
    }
}

// ====== 公开 API ======

void mqtt_init()
{
    mqtt_config_load();
    threshold_load();
    last_recon = millis();  // 延迟 10 秒首连，等 WiFi 稳定
    Serial.printf("[MQTT] 设备ID: %s\n", device_id.c_str());
}

void mqtt_upload_status()
{
    if (!mqtt_connected || !mqtt.connected()) {
        mqtt_connected = false;
        return;
    }

    bool k1 = digitalRead(key1_pin);

    unsigned long inv_adc = 4095 - light_adc;
    unsigned long lux_val = (inv_adc * inv_adc) / 30000;

    JsonDocument doc;
    doc["temperature"]     = round(current_temp * 10) / 10.0f;
    doc["light"]           = (int)lux_val;
    doc["mode"]            = auto_mode ? "auto" : "manual";
    doc["key1_lock"]       = k1;
    doc["relay3"]          = (bool)digitalRead(relay1_pin);
    doc["relay4"]          = (bool)digitalRead(relay2_pin);
    doc["temp_threshold"]  = fan_on_temp;
    doc["light_threshold"] = light_lux_th;

    String topic   = "chemctrl/" + device_id + "/status";
    String payload;
    serializeJson(doc, payload);

    bool ok = mqtt.publish(topic.c_str(), (const uint8_t*)payload.c_str(), payload.length());
    if (!ok) {
        Serial.printf("[MQTT] ↑ FAIL rc=%d\n", mqtt.state());
        mqtt_connected = false;
    }
}

void mqtt_loop()
{
    // ===== 处理 MQTT 消息 =====
    if (mqtt_connected) {
        if (!mqtt.loop()) {
            mqtt_connected = false;
            Serial.println("[MQTT] 连接断开!");
        }
    }

    // ===== WiFi 刚连上或断网 → 立即重连 MQTT =====
    static bool wifi_was = false;
    if (wifi_connected && (!wifi_was || (!mqtt_connected && millis() - last_recon > RECON_INTERVAL))) {
        if (!mqtt_connected) {
            last_recon = millis();
            mqtt_do_connect();
        }
    }
    wifi_was = wifi_connected;

    // ===== 事件触发上报：仅在状态变化时上报 =====
    if (mqtt_connected && mqtt.connected()) {
        static float  last_temp  = -99;
        static int    last_lux   = -1;
        static bool   last_k1    = false;
        static bool   last_r3    = false;
        static bool   last_r4    = false;
        static bool   last_auto  = true;

        bool k1  = digitalRead(key1_pin);
        bool r3  = digitalRead(relay1_pin);
        bool r4  = digitalRead(relay2_pin);
        unsigned long inv = 4095 - light_adc;
        int  lux_now = (inv * inv) / 30000;

        bool changed = need_upload
            || abs(current_temp - last_temp) > 0.5f
            || abs(lux_now - last_lux) > 20
            || k1 != last_k1
            || r3 != last_r3
            || r4 != last_r4
            || auto_mode != last_auto;

        if (changed) {
            last_temp = current_temp;
            last_lux  = lux_now;
            last_k1   = k1;
            last_r3   = r3;
            last_r4   = r4;
            last_auto = auto_mode;
            need_upload = false;
            mqtt_upload_status();
        }
    }
}

void mqtt_handle_command(String s)
{
    if (s.length() == 0) return;

    bool need_reconnect = false;

    if (s == "mqtt show") {
        Serial.println("=== MQTT 配置 ===");
        Serial.printf("IP:   %s\n", mqtt_ip.c_str());
        Serial.printf("Port: %d\n", mqtt_port);
        Serial.printf("User: %s\n", mqtt_user.c_str());
        Serial.printf("Pass: %s\n", mqtt_pass.c_str());
        Serial.printf("ID:   %s\n", device_id.c_str());
        Serial.printf("状态: %s\n", mqtt_connected ? "已连接" : "未连接");
        return;
    }
    if (s.startsWith("mqtt ip "))   { mqtt_ip   = s.substring(8);  mqtt_ip.trim();   mqtt_ip_global = mqtt_ip;   Serial.printf("[MQTT] IP=%s\n",     mqtt_ip.c_str());   need_reconnect = true; }
    if (s.startsWith("mqtt port ")) { mqtt_port = s.substring(10).toInt();            Serial.printf("[MQTT] Port=%d\n",   mqtt_port);         need_reconnect = true; }
    if (s.startsWith("mqtt user ")) { mqtt_user = s.substring(10); mqtt_user.trim();  Serial.printf("[MQTT] User=%s\n",   mqtt_user.c_str());  need_reconnect = true; }
    if (s.startsWith("mqtt pass ")) { mqtt_pass = s.substring(10); mqtt_pass.trim();  Serial.println(  "[MQTT] Pass=***");                     need_reconnect = true; }
    if (s.startsWith("mqtt id "))   { device_id = s.substring(8);  device_id.trim();  Serial.printf("[MQTT] ID=%s\n", device_id.c_str()); need_reconnect = true; }
    if (s == "mqtt save")           { mqtt_config_save(); threshold_save(); Serial.println("[MQTT] 已保存，将断开用新ID重连..."); need_reconnect = true; return; }
    if (s == "mqtt connect")        { mqtt.disconnect(); mqtt_connected = false; last_recon = 0; mqtt_do_connect(); return; }
    if (s == "mqtt status")         { need_upload = true; return; }

    if (need_reconnect && mqtt_connected) {
        mqtt.disconnect();
        mqtt_connected = false;
        last_recon = 0;
        mqtt_do_connect();
    }
}
