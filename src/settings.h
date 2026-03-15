#pragma once
#include <Arduino.h>

struct AppSettings {
    char     wifiSSID[33];
    char     wifiPass[65];
    uint16_t udpPort;
    bool     useMetric;
    uint8_t  brightness;      // 0-255
    float    shiftFlashPct;   // 0.0-1.0
    float    shiftGreenPct;
    float    shiftYellowPct;
    float    shiftRedPct;
};

extern AppSettings g_settings;

void settingsLoad();
void settingsSave();
