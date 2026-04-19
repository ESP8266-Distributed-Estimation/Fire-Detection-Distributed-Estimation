#pragma once
#include <Arduino.h>

// Packet sent over ESP-NOW
typedef struct struct_message {
    uint32_t nodeId;
    uint32_t seqNum;
    float temperature;
    float tempVariance;
} struct_message;

// Local entry for each discovered neighbor
typedef struct {
    uint8_t mac[6];
    uint32_t nodeId;
    uint32_t lastSeen;
    uint32_t lastSeq;
    float temperature;
    float tempVariance;
} Neighbor;
