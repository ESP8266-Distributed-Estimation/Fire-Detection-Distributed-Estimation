#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Types.h"
#include "SensorManager.h"
#include "MeshManager.h"

uint32_t lastPingTime = 0;
uint32_t lastLogTime = 0;
uint32_t seqCounter = 0;
struct_message myData;

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

        // Prepare packet
        myData.nodeId = NODE_ID;
        myData.seqNum = seqCounter++;
        myData.temperature = temp;
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
        
        float localTemp, localHum, localPres;
        SensorManager::readData(localTemp, localHum, localPres);

        MeshManager::printStatus(NODE_ID, localTemp, localHum, localPres);
    }
}
