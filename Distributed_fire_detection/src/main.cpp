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
struct_message myData;

// Kalman Filter for Temperature:
// R (Measurement Noise) = 0.5 (Sensor inaccuracy variance)
// P (Initial Error Estimate) = 1.0 
// Q (Process Noise) = 0.05 (True temp doesn't jump quickly unless fire)
// Initial value = 25.0 C (approx room temp)
KalmanFilter tempFilter(0.5f, 1.0f, 0.05f, 25.0f);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n====================================");
    Serial.println("--- Distributed Fire Detection ---");
    Serial.printf("Node ID: %d\n", NODE_ID);
    Serial.printf("MAC:     %s\n", WiFi.macAddress().c_str());
    Serial.println("====================================");

    // Prepare WiFi for ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

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
        float temp, hum, pres;
        SensorManager::readData(temp, hum, pres);

        // Filter the temperature using Kalman Filter
        float filteredTemp = tempFilter.updateEstimate(temp);
        float tempVar = tempFilter.getVariance();

        // Prepare packet
        myData.nodeId = NODE_ID;
        myData.seqNum = seqCounter++;
        myData.temperature = filteredTemp;
        myData.tempVariance = tempVar;
        myData.humidity = hum;
        myData.pressure = pres;

        // Broadcast
        MeshManager::broadcast(myData);
        Serial.print("."); // Heartbeat
    }

    // 2. Periodic Status Logging (Inbound / Table Status)
    if (now - lastLogTime >= LOG_INTERVAL_MS) {
        lastLogTime = now;

        MeshManager::cleanStaleNeighbors();
        
        float localConf = tempFilter.getConfidence();
        MeshManager::printStatus(NODE_ID, myData.temperature, localConf);
    }
}
