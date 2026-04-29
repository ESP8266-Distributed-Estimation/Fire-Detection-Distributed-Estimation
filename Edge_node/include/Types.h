#pragma once
#include <Arduino.h>

// This struct must exactly match the struct in Distributed_fire_detection
typedef struct struct_message {
    uint32_t nodeId;
    uint32_t gatewayId;    // Which gateway tree this node belongs to
    uint32_t seqNum;
    float temperature;
    float tempVariance;
    float dT;
    float dtVariance;
    uint32_t alarmSourceId;
    uint32_t alarmSeqNum;
} struct_message;