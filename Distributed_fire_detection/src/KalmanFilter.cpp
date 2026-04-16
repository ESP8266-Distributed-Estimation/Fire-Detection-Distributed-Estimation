#include "../include/KalmanFilter.h"
#include <math.h>

KalmanFilter::KalmanFilter(float mea_e, float est_e, float q, float initial_value) {
    _err_measure = mea_e;   // Sensor noise variance R
    _err_estimate = est_e;  // Initial estimation error covariance P
    _q = q;                 // Process noise variance Q
    _current_estimate = initial_value;
    _last_estimate = initial_value;
    _last_innovation = 0.0f;
}

float KalmanFilter::updateEstimate(float mea) {
    // Prediction update
    // State prediction: x_{k|k-1} = x_{k-1|k-1}
    // Estimation error covariance prediction: P_{k|k-1} = P_{k-1|k-1} + Q
    _err_estimate = _err_estimate + _q;

    // Measurement update
    // Kalman gain: K_k = P_{k|k-1} / (P_{k|k-1} + R)
    float kalman_gain = _err_estimate / (_err_estimate + _err_measure);
    
    // Innovation (residual)
    _last_innovation = mea - _last_estimate;

    // State estimate update: x_{k|k} = x_{k|k-1} + K_k(z_k - x_{k|k-1})
    _current_estimate = _last_estimate + kalman_gain * _last_innovation;
    
    // Estimation error covariance update: P_{k|k} = (1 - K_k)P_{k|k-1}
    _err_estimate = (1.0f - kalman_gain) * _err_estimate;
    
    // Setup for next iteration
    _last_estimate = _current_estimate;

    return _current_estimate;
}

float KalmanFilter::getVariance() const {
    return _err_estimate;
}

void KalmanFilter::setMeasurementError(float mea_e) {
    _err_measure = mea_e;
}

void KalmanFilter::setEstimateError(float est_e) {
    _err_estimate = est_e;
}

void KalmanFilter::setProcessNoise(float q) {
    _q = q;
}

float KalmanFilter::getConfidence() const {
    // Confidence heuristic: map variance and recent innovation to 0-100%
    // A variance of 0.0 means 100% confidence.
    // If the temperature is changing rapidly (high innovation), confidence drops.
    float lambda = 0.346f;
    float dynamic_variance = _err_estimate + (_last_innovation * _last_innovation * 2.0f); 
    float conf = 100.0f * exp(-lambda * dynamic_variance);
    // ensure confidence is within 0-100
    if(conf < 0.0f) conf = 0.0f;
    if(conf > 100.0f) conf = 100.0f;
    return conf;
}