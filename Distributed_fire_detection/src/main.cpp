#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Types.h"
#include "SensorManager.h"
#include "MeshManager.h"
#include "KalmanFilter.h"

uint32_t lastPingTime = 0;
uint32_t lastLogTime = 0;
uint32_t seqCounter = 0;
uint32_t myAlarmSeqNum = 1; // Tracks the sequence of our own generated alarms
struct_message myData;

// Kalman Filter for Temperature:
// R (Measurement Noise) = 0.5 (Sensor inaccuracy variance)
// Q_temp (Process Noise) = 0.05 (True temp drift)
// Q_dt (Process Noise for dT) = 0.01 (Velocity drift)
// Initial value = 25.0 C (approx room temp)
KalmanFilter tempFilter(0.5f, 0.05f, 0.01f, 25.0f);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n====================================");
    Serial.println("--- Distributed Fire Detection ---");
    Serial.printf("Node ID: %d\n", NODE_ID);
    Serial.printf("MAC:     %s\n", WiFi.macAddress().c_str());
    Serial.println("====================================");

    // Prepare WiFi for ESP-NOW
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ESP_MESH_HIDDEN", "12345678", WIFI_CHANNEL, 1); // Force configured channel via hidden AP
    WiFi.disconnect();
    
    // Serial.printf("Configured WiFi Channel: %d\n", wifi_get_channel());

    // Initialize modules
    SensorManager::init();
    MeshManager::init();

    Serial.println("Setup complete. Entering main loop...");
}

void loop() {
    uint32_t now = millis();

    // 1. Periodic Ping & Sensing (Outbound)
    if (now - lastPingTime >= PING_INTERVAL_MS) {
        lastPingTime = now;

        // Read sensor data
        float temp;
        SensorManager::readData(temp);

        // Filter the temperature using 2D Kalman Filter
        float filteredTemp = tempFilter.updateEstimate(temp);
        
        // 1. Evaluate current state before broadcast
        float localVar = tempFilter.getTempVariance();
        float localDt = tempFilter.getDt();
        float localDtVar = tempFilter.getDtVariance();
        
        float consensusTemp = 0.0f;
        float consensusDt = 0.0f;
        bool fireAlarm = false;
        uint32_t outAlarmSourceId = 0;
        uint32_t outAlarmSeqNum = 0;
        
        // Clean stale neighbors before consensus to avoid stale alarms
        MeshManager::cleanStaleNeighbors();
        MeshManager::cleanStaleAlarms();
        MeshManager::evaluateConsensus(NODE_ID, filteredTemp, localVar, localDt, localDtVar, consensusTemp, consensusDt, fireAlarm, outAlarmSourceId, outAlarmSeqNum);

        // Prepare packet
        myData.nodeId = NODE_ID;
        myData.seqNum = seqCounter++;
        myData.temperature = filteredTemp;
        myData.tempVariance = tempFilter.getTempVariance();
        myData.dT = tempFilter.getDt();
        myData.dtVariance = tempFilter.getDtVariance();
        
        if (fireAlarm) {
            myData.alarmSourceId = outAlarmSourceId;
            if (outAlarmSourceId == NODE_ID) {
                myData.alarmSeqNum = myAlarmSeqNum++; // Generated locally
            } else {
                myData.alarmSeqNum = outAlarmSeqNum;  // Propagated from cache
            }
        } else {
            myData.alarmSourceId = 0;
            myData.alarmSeqNum = 0;
            // DO NOT reset myAlarmSeqNum! It must only ever increase during node uptime.
        }

        // Broadcast
        MeshManager::broadcast(myData);
        Serial.print("."); // Heartbeat
    }

    // 2. Periodic Status Logging (Inbound / Table Status)
    if (now - lastLogTime >= LOG_INTERVAL_MS) {
        lastLogTime = now;
        
        float localVar = tempFilter.getTempVariance();
        float localDt = tempFilter.getDt();
        float localDtVar = tempFilter.getDtVariance();
        
        float consensusTemp = 0.0f;
        float consensusDt = 0.0f;
        bool fireAlarm = false;
        uint32_t outAlarmSourceId = 0;
        uint32_t outAlarmSeqNum = 0;
        
        // Recalculate for logging
        MeshManager::evaluateConsensus(NODE_ID, myData.temperature, localVar, localDt, localDtVar, consensusTemp, consensusDt, fireAlarm, outAlarmSourceId, outAlarmSeqNum);
        
        MeshManager::printStatus(NODE_ID, myData.temperature, localDt, consensusTemp, consensusDt, fireAlarm);
    }
}
