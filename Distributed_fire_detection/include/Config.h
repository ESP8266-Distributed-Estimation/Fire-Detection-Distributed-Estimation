#pragma once
#include <Arduino.h>

// ==============================
// --- Mesh Configuration ---
// ==============================
#define NODE_ID 1             // 🔁 CHANGE THIS PER DEVICE (1–4)
#define PING_INTERVAL_MS 2000
#define LOG_INTERVAL_MS 5000
#define MAX_NEIGHBORS 10
#define NEIGHBOR_TIMEOUT 8000

// ==============================
// --- Sensor Configuration ---
// ==============================
#define DHT_PIN 14            // GPIO14 (D5)
#define DHT_TYPE 11           // DHT11

// ==============================
// --- Kalman Calibration ---
// ==============================

struct KalmanParams {
    float R;
    float bias;
    float Q;
};

// 🔧 Per-node calibration (EDIT THESE WITH YOUR REAL VALUES)
inline KalmanParams getKalmanParams(uint8_t nodeId) {
    switch (nodeId) {
        case 1:
            return {0.12f, 0.8f, 0.00001f};

        case 2:
            return {0.10f, 0.5f, 0.00001f};

        case 3:
            return {0.15f, 1.1f, 0.00001f};

        case 4:
            return {0.11f, 0.6f, 0.00001f};

        default:
            // fallback
            return {0.12f, 0.0f, 0.00001f};
    }
}
