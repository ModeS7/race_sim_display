#pragma once
#include <Arduino.h>

// Shift mode: 0 = fixed percentage, 1 = adaptive (auto-tune power curve)
#define SHIFT_MODE_PERCENT  0
#define SHIFT_MODE_ADAPTIVE 1

struct AppSettings {
    char     wifiSSID[33];
    char     wifiPass[65];
    uint16_t udpPort;
    bool     useMetric;
    uint8_t  brightness;      // 0-255
    uint8_t  shiftMode;       // SHIFT_MODE_PERCENT or SHIFT_MODE_ADAPTIVE
    float    shiftFlashPct;   // 0.0-1.0 (used in percent mode)
    float    shiftGreenPct;
    float    shiftYellowPct;
    float    shiftRedPct;
};

extern AppSettings g_settings;

void settingsLoad();
void settingsSave();
