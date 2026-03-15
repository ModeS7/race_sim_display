#pragma once
#include "display.h"

class TelemetryScreen : public ScreenBase {
public:
    void enter() override;
    void update(const TelemetryData& data) override;
    void leave() override;

private:
    void drawStatic();

    // G-force dot
    int prevDotX = -1;
    int prevDotY = -1;

    // Numeric readouts
    int prevSpeed = -1;
    int prevRpm = -1;
    int prevPower = -1;
    int prevTorque = -1;

    static const int GF_CX = 80;   // G-force circle center X
    static const int GF_CY = 115;  // G-force circle center Y
    static const int GF_R  = 70;   // G-force circle radius
    static const int DOT_R = 4;    // dot radius
};
