#pragma once
#include <Arduino.h>

// --- Edge Node Identity ---
#define EDGE_NODE_ID 200

// --- WiFi Configuration ---
// IMPORTANT: For ESP8266 to use ESP-NOW and WiFi simultaneously, 
// the WiFi channel MUST match your router's channel exactly!
#define WIFI_SSID "Your_SSID_Here"
#define WIFI_PASSWORD "Your_Password_Here"
#define WIFI_CHANNEL 1 

// --- MQTT Configuration ---
#define MQTT_SERVER "Your_MQTT_Server_IP"  // Updated to laptop's IP
#define MQTT_PORT 1883
#define MQTT_USER ""                 // Leave blank if no auth
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_FIRE "fire_alarm/alerts"
#define MQTT_TOPIC_STATUS "fire_alarm/edge_status"

// --- System Configuration ---
#define STATUS_INTERVAL_MS 10000     // How often the edge node pings MQTT that it's alive
#define MAX_ACTIVE_ALARMS 20         // Size of the de-duplication cache
#define NODE_TIMEOUT_MS 10000        // Remove sensor node if no packet received in 10 seconds
