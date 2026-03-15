#pragma once
#include "display.h"

class LapsScreen : public ScreenBase {
public:
    void enter() override;
    void update(const TelemetryData& data) override;
    void leave() override;

private:
    void drawStatic();
    void formatTime(float seconds, char* buf, size_t len);

    int   prevCurrentLapInt = -1;
    int   prevBestLapInt = -1;
    int   prevLastLapInt = -1;
    int   prevPosition = -1;
    int   prevGear = -1;
    int   prevSpeed = -1;
};
