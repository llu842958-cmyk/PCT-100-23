#ifndef __MQTT_MANAGER_H__
#define __MQTT_MANAGER_H__

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

extern bool   mqtt_connected;
extern String  mqtt_ip_global;

void mqtt_init();
void mqtt_loop();
void mqtt_upload_status();
void mqtt_handle_command(String s);

#endif
