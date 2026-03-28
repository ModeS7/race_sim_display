#pragma once
#include "display.h"

class SettingsScreen : public ScreenBase {
public:
    void enter() override;
    void update(const TelemetryData& data) override;
    void leave() override;
    bool handleTouch(int x, int y) override;

private:
    void drawStatic();
    void drawRow(int row);

    static const int NUM_ROWS = 6;
    static const int ROW_H = 32;
    static const int ROW_Y0 = 28;
};
