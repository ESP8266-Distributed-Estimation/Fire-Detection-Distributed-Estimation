#pragma once
#include <Arduino.h>

// --- Scheme Constants ---
#define SCHEME_DIFFUSION 0
#define SCHEME_ITERATIVE 1

// --- Command Types ---
#define CMD_SET_SCHEME 1

// This struct must exactly match the struct in Distributed_fire_detection
typedef struct __attribute__((packed)) struct_message {
  uint32_t nodeId;
  uint32_t gatewayId; // Which gateway tree this node belongs to
  uint32_t seqNum;
  float temperature;
  float tempVariance;
  float dT;
  float dtVariance;
  uint32_t alarmSourceId;
  uint32_t alarmSeqNum;
  uint8_t scheme; // Current active scheme on this node
} struct_message;

// Command packet sent from gateway to sensor nodes via ESP-NOW
typedef struct __attribute__((packed)) command_message {
  uint32_t gatewayId;
  uint8_t commandType;
  uint8_t schemeType;
} command_message;