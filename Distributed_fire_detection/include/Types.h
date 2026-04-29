#pragma once
#include <Arduino.h>

// --- Scheme Constants ---
#define SCHEME_DIFFUSION 0
#define SCHEME_ITERATIVE 1

// --- Command Types ---
#define CMD_SET_SCHEME 1

// Packet sent over ESP-NOW (data broadcast from sensor nodes)
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
  uint8_t scheme; // Current active scheme on this node (SCHEME_DIFFUSION or SCHEME_ITERATIVE)
} struct_message;

// Command packet sent from gateway to sensor nodes via ESP-NOW
typedef struct __attribute__((packed)) command_message {
  uint32_t gatewayId;   // Tree isolation: only nodes matching this gateway process it
  uint8_t commandType;  // CMD_SET_SCHEME, etc.
  uint8_t schemeType;   // SCHEME_DIFFUSION or SCHEME_ITERATIVE
} command_message;

// Local entry for each discovered neighbor
typedef struct {
  uint8_t mac[6];
  uint32_t nodeId;
  uint32_t lastSeen;
  uint32_t lastSeq;
  float temperature;
  float tempVariance;
  float dT;
  float dtVariance;
} Neighbor;
