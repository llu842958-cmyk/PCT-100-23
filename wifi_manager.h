#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <Arduino.h>
#include <WiFi.h>

enum WifiState {
    WF_OFF,
    WF_SCANNING,
    WF_SELECTING,
    WF_CONNECTING,
    WF_CONNECTED,
    WF_DISCONNECTED
};

extern WifiState wifi_state;
extern String   wifi_ip;
extern bool     wifi_connected;

void wifi_init();
void wifi_connect();     // KEY1 ON  → 连接
void wifi_disconnect();  // KEY1 OFF → 断开
void wifi_loop();
void wifi_handle_command(String s);

#endif
