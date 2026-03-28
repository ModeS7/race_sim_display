#pragma once
#include <TFT_eSPI.h>
#include "telemetry.h"

#define SCREEN_W 320
#define SCREEN_H 240

#define LED_R 4
#define LED_G 16
#define LED_B 17
#define LDR_PIN 34

extern TFT_eSPI tft;

void displayInit();
void setLED(bool r, bool g, bool b);
void drawPageDots(int currentPage, int totalPages);
void updateAutoBrightness();

// RPM bar gradient helper — returns color for a given position fraction (0.0-1.0)
uint16_t rpmBarColor(float fraction);

// Adaptive tire temp tracking and coloring
void tireTempUpdate(const TelemetryData& d);
uint16_t tireTempColor(float tempF);

// Abstract screen interface
class ScreenBase {
public:
    virtual void enter() = 0;
    virtual void update(const TelemetryData& data) = 0;
    virtual void leave() = 0;
    virtual bool handleTouch(int x, int y) { return false; }  // return true if handled
    virtual ~ScreenBase() = default;
};
