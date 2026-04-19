#include "../include/KalmanFilter.h"
#include <math.h>

// Constructor
KalmanFilter::KalmanFilter(float mea_e, float est_e, float q, float initial_value) {
    _err_measure = mea_e;   // Sensor noise variance R
    _err_estimate = est_e;  // Initial estimation error covariance P
    _q = q;                 // Process noise variance Q
    _current_estimate = initial_value;
    _last_estimate = initial_value;
    _last_innovation = 0.0f;
    _bias = 0.0f;           // default no bias
}

// Update step
float KalmanFilter::updateEstimate(float mea) {
    // Prediction update
    _err_estimate = _err_estimate + _q;

    // Kalman gain
    float kalman_gain = _err_estimate / (_err_estimate + _err_measure);
    
    // Apply bias correction
    float corrected_mea = mea - _bias;

    // Innovation (residual)
    _last_innovation = corrected_mea - _last_estimate;

    // State update
    _current_estimate = _last_estimate + kalman_gain * _last_innovation;
    
    // Covariance update
    _err_estimate = (1.0f - kalman_gain) * _err_estimate;
    
    // Store for next iteration
    _last_estimate = _current_estimate;

    return _current_estimate;
}

// Get variance (uncertainty)
float KalmanFilter::getVariance() const {
    return _err_estimate;
}

// Set measurement noise R
void KalmanFilter::setMeasurementError(float mea_e) {
    _err_measure = mea_e;
}

// Set estimation error P
void KalmanFilter::setEstimateError(float est_e) {
    _err_estimate = est_e;
}

// Set process noise Q
void KalmanFilter::setProcessNoise(float q) {
    _q = q;
}

// Set bias (NEW)
void KalmanFilter::setBias(float bias) {
    _bias = bias;
}

// Confidence estimation
float KalmanFilter::getConfidence() const {
    // Confidence heuristic: map variance and innovation to 0–100%
    float lambda = 0.346f;
    float dynamic_variance = _err_estimate + (_last_innovation * _last_innovation * 2.0f); 
    float conf = 100.0f * exp(-lambda * dynamic_variance);

    if(conf < 0.0f) conf = 0.0f;
    if(conf > 100.0f) conf = 100.0f;

    return conf;
}
