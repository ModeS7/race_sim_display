#pragma once
#include <TFT_eSPI.h>
#include "telemetry.h"

#define SCREEN_W 320
#define SCREEN_H 240

#define LED_R 4
#define LED_G 16
#define LED_B 17

extern TFT_eSPI tft;

void displayInit();
void setLED(bool r, bool g, bool b);
void drawPageDots(int currentPage, int totalPages);

// Abstract screen interface
class ScreenBase {
public:
    virtual void enter() = 0;
    virtual void update(const TelemetryData& data) = 0;
    virtual void leave() = 0;
    virtual bool handleTouch(int x, int y) { return false; }  // return true if handled
    virtual ~ScreenBase() = default;
};
