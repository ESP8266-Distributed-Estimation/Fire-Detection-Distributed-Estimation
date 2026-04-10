#include "MeshManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

namespace MeshManager {
    Neighbor neighbors[MAX_NEIGHBORS];
    int neighborCount = 0;
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    void printMac(const uint8_t *mac) {
        for (int i = 0; i < 6; i++) {
            Serial.printf("%02X", mac[i]);
            if (i < 5) Serial.print(":");
        }
    }

    int findNeighbor(const uint8_t *mac) {
        for (int i = 0; i < neighborCount; i++) {
            if (memcmp(neighbors[i].mac, mac, 6) == 0) return i;
        }
        return -1;
    }

    void updateNeighbor(const uint8_t *mac, const struct_message *data) {
        int idx = findNeighbor(mac);
        if (idx != -1) {
            // Existing neighbor: update their record
            neighbors[idx].lastSeen = millis();
            neighbors[idx].lastSeq = data->seqNum;
            neighbors[idx].nodeId = data->nodeId;
            neighbors[idx].temperature = data->temperature;
            neighbors[idx].humidity = data->humidity;
            neighbors[idx].pressure = data->pressure;
        } else if (neighborCount < MAX_NEIGHBORS) {
            // New neighbor: register them
            memcpy(neighbors[neighborCount].mac, mac, 6);
            neighbors[neighborCount].nodeId = data->nodeId;
            neighbors[neighborCount].lastSeen = millis();
            neighbors[neighborCount].lastSeq = data->seqNum;
            neighbors[neighborCount].temperature = data->temperature;
            neighbors[neighborCount].humidity = data->humidity;
            neighbors[neighborCount].pressure = data->pressure;
            
            Serial.print("\n[DISCOVERED] Node ID: ");
            Serial.print(data->nodeId);
            Serial.print(" MAC: ");
            printMac(mac);
            Serial.println();
            neighborCount++;
        }
    }

    void cleanStaleNeighbors() {
        uint32_t now = millis();
        for (int i = 0; i < neighborCount; i++) {
            if (now - neighbors[i].lastSeen > NEIGHBOR_TIMEOUT) {
                Serial.printf("\n[TIMEOUT] Node %d removed\n", neighbors[i].nodeId);
                // Shift remaining neighbors up
                for (int j = i; j < neighborCount - 1; j++) {
                    neighbors[j] = neighbors[j + 1];
                }
                neighborCount--;
                i--; // Re-check this index
            }
        }
    }

    // Callbacks must be global/static in ESP8266 ESP-NOW
    void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
        if (len != sizeof(struct_message)) return;
        struct_message recvData;
        memcpy(&recvData, incomingData, sizeof(recvData));
        updateNeighbor(mac, &recvData);
    }

    void OnDataSent(uint8_t *mac_addr, uint8_t status) {
        // Optional debug logging
    }

    void init() {
        if (esp_now_init() != 0) {
            Serial.println("ESP-NOW Init Failed");
            return;
        }
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        esp_now_register_send_cb(OnDataSent);
        esp_now_register_recv_cb(OnDataRecv);
        esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
        Serial.println("ESP-NOW Mesh Initialized.");
    }

    void broadcast(struct_message &data) {
        esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));
    }

    void printStatus(uint32_t localNodeId, float localTemp, float localHum, float localPres) {
        uint32_t uptimeSec = millis() / 1000;
        Serial.printf("\n[STATUS] Node: %d | Uptime: %u s | Neighbors: %d\n", localNodeId, uptimeSec, neighborCount);
        Serial.printf("  -> Local Sensor: Temp=%.2f*C, Hum=%.2f%%, Pres=%.2fhPa\n", localTemp, localHum, localPres);

        if (neighborCount > 0) {
            Serial.println("--- Neighbor List ---");
            for (int i = 0; i < neighborCount; i++) {
                Serial.printf("  ID: %d | Temp: %.2f*C | Hum: %.2f%% | Seq: %u | MAC: ", 
                              neighbors[i].nodeId, neighbors[i].temperature, neighbors[i].humidity, neighbors[i].lastSeq);
                printMac(neighbors[i].mac);
                Serial.println();
            }
            Serial.println("---------------------");
        }
    }
}
