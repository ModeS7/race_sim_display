#pragma once
#include "display.h"

class DashExtScreen : public ScreenBase {
public:
    void enter() override;
    void update(const TelemetryData& data) override;
    void leave() override;

private:
    void drawStatic();
    void resetState();

    int      prevSpeed = -1;
    int      prevGear = -1;
    int      prevRpmBarW = -1;
    int      prevRpm = -1;
    float    prevBoost = -999.0f;
    int      prevPower = -1;
    int      prevTorque = -1;
    int      prevAccelBar = -1;
    int      prevBrakeBar = -1;
    int      prevPosition = -1;
    int      prevCurrentLap = -1;
    int      prevBestLap = -1;
    int      prevLastLap = -1;
    bool     prevFlashState = false;
    bool     wasFlashing = false;
    uint32_t lastFlashToggle = 0;
    bool     flashOn = false;
};
