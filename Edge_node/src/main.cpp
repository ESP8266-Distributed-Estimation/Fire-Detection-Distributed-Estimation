#include <Arduino.h>
#include "Config.h"
#include "CacheManager.h"
#include "NetworkManager.h"

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n====================================");
    Serial.println("--- Edge Node Gateway (Layer 2) ---");
    Serial.printf("Gateway ID: %d\n", EDGE_NODE_ID);
    Serial.println("====================================");

    CacheManager::init();
    NetworkManager::init();
    
    Serial.println("Gateway running. Listening for Mesh Alarms...");
}

void loop() {
    NetworkManager::loop();
    NetworkManager::processQueuedPackets();
}