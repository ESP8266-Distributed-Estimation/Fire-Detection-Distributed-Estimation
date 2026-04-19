#pragma once
#include <Arduino.h>

class KalmanFilter {
private:
    float _r;       // Measurement noise (R)
    float _q_temp;  // Process noise for Temperature
    float _q_dt;    // Process noise for dT

    float _x_temp;  // State 0: Temperature
    float _x_dt;    // State 1: Rate of Change

    float _p[2][2]; // Covariance matrix P

    uint32_t _last_time;
    bool _is_initialized;

public:
    KalmanFilter(float r, float q_temp, float q_dt, float initial_temp);
    
    // Updates the state and returns the estimated temperature
    float updateEstimate(float mea);
    
    float getTempVariance() const;
    float getDtVariance() const;
    float getDt() const;
    float getConfidence() const; // Legacy heuristic for UI if needed
};