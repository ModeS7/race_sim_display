#pragma once
#include "telemetry.h"

enum ShiftAdvice { SHIFT_NONE, SHIFT_DOWNSHIFT, SHIFT_UPSHIFT };

class ShiftAdvisor {
public:
    // Call every frame with current telemetry
    void update(const TelemetryData& d);

    // Get current shift advice (works in both modes)
    ShiftAdvice getAdvice() const { return advice; }

    // Get the learned peak power RPM (adaptive mode)
    float getPeakPowerRpm() const { return peakPowerRpm; }

    // Should the full-screen flash trigger?
    bool shouldFlash() const { return advice == SHIFT_UPSHIFT; }

private:
    // Adaptive learning
    float peakPowerRpm = 0.0f;
    float peakPowerSeen = 0.0f;
    float lastEngineMaxRpm = 0.0f;  // detect car change

    // Current state
    ShiftAdvice advice = SHIFT_NONE;
};

extern ShiftAdvisor g_shiftAdvisor;
