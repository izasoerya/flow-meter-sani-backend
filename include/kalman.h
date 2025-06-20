// KalmanFilter.h
#pragma once

class KalmanFilter
{
private:
    float Q; // Process noise covariance
    float R; // Measurement noise covariance
    float P; // Estimation error covariance
    float X; // Value estimate
    bool initialized = false;

public:
    KalmanFilter(float processNoise = 0.01, float measurementNoise = 2.08)
        : Q(processNoise), R(measurementNoise), P(1.0), X(0.0) {}

    void reset()
    {
        initialized = false;
        P = 1.0;
        X = 0.0;
    }

    float filter(float measurement)
    {
        if (!initialized)
        {
            X = measurement;
            initialized = true;
        }

        // Prediction update
        P = P + Q;

        // Measurement update
        float K = P / (P + R);         // Kalman Gain
        X = X + K * (measurement - X); // Update estimate
        P = (1 - K) * P;

        return X;
    }

    void setParameters(float q, float r)
    {
        Q = q;
        R = r;
    }

    float getEstimate() const
    {
        return X;
    }
};
