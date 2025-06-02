#pragma once
#include <Arduino.h>

class FlowMeter
{
private:
    volatile uint32_t pulseCount = 0;

public:
    void IRAM_ATTR incrementPulseCount()
    {
        pulseCount++;
    }

    void resetPulseCount()
    {
        noInterrupts();
        pulseCount = 0;
        interrupts();
    }

    uint32_t getPulseCount()
    {
        noInterrupts();
        uint32_t count = pulseCount;
        interrupts();
        return count;
    }

    float getFlowRateLPM(unsigned long durationMs)
    {
        if (durationMs == 0)
            return 0.0f;
        uint32_t count = getPulseCount();
        return (count * 1000.0f) / (durationMs * 0.45f);
    }
};