// flowmeter.h
#pragma once

#include <Arduino.h>

class FlowMeter
{
private:
    volatile uint32_t pulseCount = 0;
    const float volumePerPulse = 7.407f; // YF-DN50: Each pulse ≈ 7.407 mL

public:
    void incrementPulseCount()
    {
        pulseCount++;
    }

    void resetPulseCount()
    {
        pulseCount = 0;
    }

    uint32_t getPulseCount()
    {
        return pulseCount;
    }

    float getVolumeMilliLiters()
    {
        return pulseCount * volumePerPulse;
    }
};
