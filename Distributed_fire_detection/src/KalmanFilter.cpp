#include "../include/KalmanFilter.h"
#include <math.h>

KalmanFilter::KalmanFilter(float r, float q_temp, float q_dt, float initial_temp) {
    _r = r;
    _q_temp = q_temp;
    _q_dt = q_dt;
    
    _x_temp = initial_temp;
    _x_dt = 0.0f;
    
    // Initial uncertainty
    _p[0][0] = 1.0f;
    _p[0][1] = 0.0f;
    _p[1][0] = 0.0f;
    _p[1][1] = 1.0f;
    
    _last_time = 0;
    _is_initialized = false;
}

float KalmanFilter::updateEstimate(float mea) {
    uint32_t now = millis();
    
    if (!_is_initialized) {
        _last_time = now;
        _is_initialized = true;
        _x_temp = mea;
        return _x_temp;
    }
    
    float dt = (now - _last_time) / 1000.0f; // time delta in seconds
    if (dt <= 0.0f) dt = 0.001f;
    _last_time = now;

    // --- PREDICT ---
    // X_pred = F * X
    float x_temp_pred = _x_temp + dt * _x_dt;
    float x_dt_pred = _x_dt;

    // P_pred = F * P * F^T + Q
    float p00_pred = _p[0][0] + dt * (_p[1][0] + _p[0][1]) + dt * dt * _p[1][1] + _q_temp;
    float p01_pred = _p[0][1] + dt * _p[1][1];
    float p10_pred = _p[1][0] + dt * _p[1][1];
    float p11_pred = _p[1][1] + _q_dt;

    // --- UPDATE ---
    // Innovation y = Z - H * X_pred
    float y = mea - x_temp_pred;
    
    // Innovation covariance S = H * P_pred * H^T + R
    float S = p00_pred + _r;
    
    // Kalman Gain K = P_pred * H^T * S^-1
    float K0 = p00_pred / S;
    float K1 = p10_pred / S;

    // State update X = X_pred + K * y
    _x_temp = x_temp_pred + K0 * y;
    _x_dt = x_dt_pred + K1 * y;

    // Covariance update P = (I - K * H) * P_pred
    _p[0][0] = (1.0f - K0) * p00_pred;
    _p[0][1] = (1.0f - K0) * p01_pred;
    _p[1][0] = -K1 * p00_pred + p10_pred;
    _p[1][1] = -K1 * p01_pred + p11_pred;

    return _x_temp;
}

float KalmanFilter::getTempVariance() const {
    return _p[0][0];
}

float KalmanFilter::getDtVariance() const {
    return _p[1][1];
}

float KalmanFilter::getDt() const {
    return _x_dt;
}

float KalmanFilter::getConfidence() const {
    // Keep a simple heuristic based on temp variance for UI if needed
    float lambda = 0.346f;
    float conf = 100.0f * exp(-lambda * _p[0][0]);
    if(conf < 0.0f) conf = 0.0f;
    if(conf > 100.0f) conf = 100.0f;
    return conf;
}