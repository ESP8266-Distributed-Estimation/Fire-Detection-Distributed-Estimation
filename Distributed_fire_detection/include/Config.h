#pragma once
#include <Arduino.h>

// --- Mesh Configuration ---
#define NODE_ID 102           // Unique ID for this node
#define PING_INTERVAL_MS 2000 // Send ping every 2 seconds
#define LOG_INTERVAL_MS 5000  // Log status every 5 seconds
#define MAX_NEIGHBORS 10      // Max neighbors to track
#define NEIGHBOR_TIMEOUT 8000 // Remove neighbor after 8 seconds of silence

#define MAX_ACTIVE_ALARMS 20  // Max simultaneous alarm sources to track
#define ALARM_TIMEOUT_MS 8000 // Expire propagated alarm after 8 seconds of no updates
#define ALARM_FORGET_MS 20000 // Completely forget sequence numbers after 20s to prevent zombie echoes

// --- Sensor Configuration ---
#define BME280_I2C_ADDR 0x76  // Default I2C address for GY-BME280 (can be 0x77)

// --- Fire Detection Thresholds ---
#define CRITICAL_TEMP 50.0f     // Absolute alarm temperature
#define BASELINE_TEMP 35.0f     // Must be at least this hot to trigger fast-rise alarm
#define CRITICAL_DT 0.2f        // Critical rate of change (deg C per second)
