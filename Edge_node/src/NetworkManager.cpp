#include "NetworkManager.h"
#include "Config.h"
#include "CacheManager.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <queue>

namespace NetworkManager {
    WiFiClient espClient;
    PubSubClient mqttClient(espClient);
    
    uint32_t lastStatusTime = 0;

    // --- Interrupt-safe Queue for ESP-NOW packets ---
    // Using a simple ring buffer structure as std::queue isn't strictly interrupt safe
    #define QUEUE_SIZE 10
    volatile int queueHead = 0;
    volatile int queueTail = 0;
    struct_message packetQueue[QUEUE_SIZE];

    void pushToQueue(const struct_message* data) {
        int nextHead = (queueHead + 1) % QUEUE_SIZE;
        if (nextHead != queueTail) { // Not full
            memcpy((void*)&packetQueue[queueHead], data, sizeof(struct_message));
            queueHead = nextHead;
        }
    }

    bool popFromQueue(struct_message* data) {
        if (queueHead == queueTail) return false; // Empty
        memcpy(data, (void*)&packetQueue[queueTail], sizeof(struct_message));
        queueTail = (queueTail + 1) % QUEUE_SIZE;
        return true;
    }

    // --- Callbacks ---
    void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
        if (len != sizeof(struct_message)) return;
        struct_message recvData;
        memcpy(&recvData, incomingData, sizeof(recvData));
        pushToQueue(&recvData);
    }

    void connectWiFi() {
        Serial.printf("Connecting to WiFi %s on channel %d...\n", WIFI_SSID, WIFI_CHANNEL);
        
        // Critical for STA + ESP-NOW: Set channel explicitly
        WiFi.mode(WIFI_STA);
        wifi_promiscuous_enable(1);
        wifi_set_channel(WIFI_CHANNEL);
        wifi_promiscuous_enable(0);
        
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi connected.");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("\nWiFi connection failed! Will retry in loop.");
        }
    }

    void connectMQTT() {
        if (WiFi.status() != WL_CONNECTED) return;
        
        while (!mqttClient.connected()) {
            Serial.print("Attempting MQTT connection...");
            String clientId = "EdgeNode-";
            clientId += String(EDGE_NODE_ID);
            
            if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
                Serial.println("connected");
            } else {
                Serial.print("failed, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" try again in 5 seconds");
                delay(5000);
            }
        }
    }

    void init() {
        connectWiFi();
        
        mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
        
        if (esp_now_init() != 0) {
            Serial.println("ESP-NOW Init Failed");
            return;
        }
        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE); // Listen only
        esp_now_register_recv_cb(OnDataRecv);
        
        Serial.println("Network Subsystem Initialized.");
    }

    void loop() {
        if (WiFi.status() != WL_CONNECTED) {
            connectWiFi();
        } else if (!mqttClient.connected()) {
            connectMQTT();
        }
        mqttClient.loop();

        uint32_t now = millis();
        if (now - lastStatusTime >= STATUS_INTERVAL_MS) {
            lastStatusTime = now;
            if (mqttClient.connected()) {
                JsonDocument doc;
                doc["edge_node_id"] = EDGE_NODE_ID;
                doc["status"] = "online";
                doc["uptime"] = now / 1000;
                
                String output;
                serializeJson(doc, output);
                mqttClient.publish(MQTT_TOPIC_STATUS, output.c_str());
            }
        }
    }

    void publishAlarmToMQTT(const struct_message& data) {
        if (!mqttClient.connected()) return;

        JsonDocument doc;
        doc["edge_node_id"] = EDGE_NODE_ID;
        doc["fire_source_id"] = data.alarmSourceId;
        doc["alarm_sequence"] = data.alarmSeqNum;
        doc["reporting_node"] = data.nodeId;
        doc["raw_temperature"] = data.temperature;
        doc["raw_dT"] = data.dT;

        String output;
        serializeJson(doc, output);
        
        if(mqttClient.publish(MQTT_TOPIC_FIRE, output.c_str())) {
            Serial.printf("[MQTT] Published Alarm: Source %u, Seq %u\n", data.alarmSourceId, data.alarmSeqNum);
        } else {
            Serial.println("[MQTT] Failed to publish alarm!");
        }
    }

    void processQueuedPackets() {
        struct_message packet;
        while (popFromQueue(&packet)) {
            // Did the incoming packet contain an active fire alarm?
            if (packet.alarmSourceId != 0) {
                // Have we already published this exact fire sequence?
                if (CacheManager::isNewAlarm(packet.alarmSourceId, packet.alarmSeqNum)) {
                    Serial.printf("\n[NEW ALARM CAUGHT] Source: %u | Seq: %u\n", packet.alarmSourceId, packet.alarmSeqNum);
                    publishAlarmToMQTT(packet);
                }
            }
        }
    }
}