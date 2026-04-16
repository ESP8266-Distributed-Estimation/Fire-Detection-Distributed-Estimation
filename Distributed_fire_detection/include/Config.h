#pragma once
#include <Arduino.h>

// --- Mesh Configuration ---
#define NODE_ID 103           // Unique ID for this node
#define PING_INTERVAL_MS 2000 // Send ping every 2 seconds
#define LOG_INTERVAL_MS 5000  // Log status every 5 seconds
#define MAX_NEIGHBORS 10      // Max neighbors to track
#define NEIGHBOR_TIMEOUT 8000 // Remove neighbor after 8 seconds of silence

// --- Sensor Configuration ---
#define BME280_I2C_ADDR 0x76  // Default I2C address for GY-BME280 (can be 0x77)
