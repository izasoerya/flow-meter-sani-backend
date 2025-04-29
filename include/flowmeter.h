#pragma once

#include <Arduino.h>

class FlowMeter
{
private:
    uint32_t pulseCount = 0;

public:
    FlowMeter() {};
    ~FlowMeter() {};

    void incrementPulseCount()
    {
        pulseCount++;
    }

    uint32_t getPulseCount()
    {
        return pulseCount;
    }

    void resetPulseCount()
    {
        pulseCount = 0;
    }
};