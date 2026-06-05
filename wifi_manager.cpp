#include "wifi_manager.h"
#include <Preferences.h>

WifiState wifi_state     = WF_OFF;
String    wifi_ip        = "";
bool      wifi_connected = false;

static Preferences prefs;
static String ssid;
static String pass;
static bool save_cred = false;

static String nets[20];
static int    net_n    = 0;
static bool   scanning = false;
static bool   need_pwd = false;

static bool          want_connect = false;
static unsigned long conn_t       = 0;
static bool          did_begin    = false;

static void flash_save() {
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
    Serial.println("[WiFi] 已保存到Flash");
}

static bool flash_load() {
    prefs.begin("wifi", true);
    ssid = prefs.getString("ssid", "");
    pass = prefs.getString("pass", "");
    prefs.end();
    return ssid.length() > 0;
}

static void wifi_scan() {
    WiFi.disconnect(true);
    delay(50);
    net_n = WiFi.scanNetworks();
    if (net_n > 20) net_n = 20;
    for (int i = 0; i < net_n; i++) nets[i] = WiFi.SSID(i);

    Serial.println();
    Serial.println("=== 可用网络 ===");
    for (int i = 0; i < net_n; i++) {
        Serial.printf(" %d: %s (%ddBm)%s\n",
            i+1, nets[i].c_str(), WiFi.RSSI(i),
            WiFi.encryptionType(i)==WIFI_AUTH_OPEN?" [无密码]":"");
    }
    Serial.println("================");
    Serial.println("输入编号选择网络，输入 s 重新扫描");
    wifi_state = WF_SELECTING;
    scanning   = true;
}

void wifi_init() {
    WiFi.mode(WIFI_STA);
    if (!flash_load()) {
        ssid = "普通市民";
        pass = "1234567890";
    }
    Serial.print("[WiFi] 已就绪: ");
    Serial.println(ssid);
    wifi_connect();  // 自动连接
}

void wifi_connect() {
    if (wifi_connected) return;
    if (wifi_state == WF_CONNECTING) return;
    want_connect = true;
    save_cred    = false;
}

void wifi_disconnect() {
    want_connect   = false;
    save_cred      = false;
    wifi_connected = false;
    wifi_ip        = "";
    if (wifi_state == WF_CONNECTING || wifi_state == WF_CONNECTED) {
        WiFi.disconnect(true);
        delay(50);
    }
    wifi_state = WF_DISCONNECTED;
}

void wifi_loop() {
    if (want_connect) {
        want_connect = false;
        wifi_state   = WF_CONNECTING;
        conn_t       = millis();
        did_begin    = false;
        Serial.print("[WiFi] 连接中 ");
        Serial.print(ssid);
        Serial.println("...");
    }

    if (wifi_state == WF_CONNECTING) {
        if (!did_begin) {
            did_begin = true;
            WiFi.disconnect(true);
            delay(50);
            WiFi.begin(ssid.c_str(), pass.c_str());
        }

        if (WiFi.status() == WL_CONNECTED) {
            wifi_ip        = WiFi.localIP().toString();
            wifi_connected = true;
            wifi_state     = WF_CONNECTED;
            Serial.print("[WiFi] 连接成功 IP=");
            Serial.println(wifi_ip);
            if (save_cred) { flash_save(); save_cred = false; }
        }
        else if (millis() - conn_t > 15000) {
            wifi_connected = false;
            wifi_state     = WF_DISCONNECTED;
            Serial.println("[WiFi] 连接失败，输入 s 扫描选网");
        }
    }

    if (wifi_state == WF_CONNECTED && WiFi.status() != WL_CONNECTED) {
        wifi_connected = false;
        wifi_ip        = "";
        wifi_state     = WF_DISCONNECTED;
        Serial.println("[WiFi] 已断开");
    }
}

void wifi_handle_command(String s) {
    if (s.length() == 0) return;

    if (need_pwd) {
        pass      = s;
        need_pwd  = false;
        save_cred = true;
        want_connect = true;
        Serial.println("[WiFi] 密码已设置，连接中...");
        return;
    }

    if (s == "s" || s == "S") {
        wifi_disconnect();
        wifi_scan();
        return;
    }

    if (scanning) {
        int i = s.toInt();
        if (i >= 1 && i <= net_n) {
            ssid     = nets[i-1];
            scanning = false;
            need_pwd = true;
            Serial.print("已选择: ");
            Serial.println(ssid);
            Serial.println("请输入密码:");
        } else {
            Serial.println("无效编号，请重试");
        }
    }
}
