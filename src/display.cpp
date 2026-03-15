#include "display.h"
#include "settings.h"

TFT_eSPI tft = TFT_eSPI();

void displayInit() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    setLED(false, false, false);

    pinMode(TFT_BL, OUTPUT);
    analogWrite(TFT_BL, g_settings.brightness);

    tft.init();
    tft.setRotation(1);
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);
}

void setLED(bool r, bool g, bool b) {
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
}

void drawPageDots(int currentPage, int totalPages) {
    const int dotR = 3;
    const int dotSpacing = 14;
    const int dotY = SCREEN_H - 8;
    int totalW = (totalPages - 1) * dotSpacing;
    int startX = (SCREEN_W - totalW) / 2;

    // Clear dot area
    tft.fillRect(startX - dotR - 2, dotY - dotR - 1, totalW + dotR * 2 + 4, dotR * 2 + 2, TFT_BLACK);

    for (int i = 0; i < totalPages; i++) {
        int cx = startX + i * dotSpacing;
        if (i == currentPage) {
            tft.fillCircle(cx, dotY, dotR, TFT_WHITE);
        } else {
            tft.drawCircle(cx, dotY, dotR, TFT_DARKGREY);
        }
    }
}
