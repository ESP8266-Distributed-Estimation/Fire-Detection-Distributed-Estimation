#pragma once

class KalmanFilter {
private:
    float _err_measure; // Measurement uncertainty (R)
    float _err_estimate; // Estimation uncertainty (P)
    float _q;            // Process noise (Q)
    float _current_estimate; // State (x)
    float _last_estimate;    // Previous state
    float _last_innovation;  // Difference between measurement and prediction
    float _bias; // Bias

public:
    KalmanFilter(float mea_e, float est_e, float q, float initial_value);
    
    float updateEstimate(float mea);
    float getVariance() const;
    void setMeasurementError(float mea_e);
    void setEstimateError(float est_e);
    void setProcessNoise(float q);
    float getConfidence() const; // Returns a 0-100% confidence heuristic based on variance
    void setBias(float bias);
};
