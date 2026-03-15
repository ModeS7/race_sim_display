#include "screen_laps.h"
#include "settings.h"

void LapsScreen::enter() {
    prevCurrentLapInt = -1;
    prevBestLapInt = -1;
    prevLastLapInt = -1;
    prevPosition = -1;
    prevGear = -1;
    prevSpeed = -1;
    drawStatic();
}

void LapsScreen::leave() {}

void LapsScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("CURRENT LAP", 160, 10);

    tft.setTextDatum(TL_DATUM);
    tft.drawString("BEST LAP", 10, 90);
    tft.drawString("LAST LAP", 10, 130);
    tft.drawString("POSITION", 10, 170);

    // Speed & gear labels (right side, lower)
    tft.drawString("SPEED", 200, 90);
    tft.drawString("GEAR", 200, 130);
}

void LapsScreen::formatTime(float seconds, char* buf, size_t len) {
    if (seconds <= 0.0f) {
        snprintf(buf, len, "--:--.---");
        return;
    }
    int mins = (int)(seconds / 60.0f);
    float secs = seconds - (mins * 60.0f);
    snprintf(buf, len, "%d:%06.3f", mins, secs);
}

void LapsScreen::update(const TelemetryData& d) {
    char buf[16];

    // ── Current lap time (large, center) ────────────────────────────────
    int curInt = (int)(d.currentLap * 1000);
    if (curInt != prevCurrentLapInt) {
        formatTime(d.currentLap, buf, sizeof(buf));
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(10, 25, 300, 50, TFT_BLACK);
        tft.drawString(buf, 160, 50);
        prevCurrentLapInt = curInt;
    }

    // ── Best lap ────────────────────────────────────────────────────────
    int bestInt = (int)(d.bestLap * 1000);
    if (bestInt != prevBestLapInt) {
        formatTime(d.bestLap, buf, sizeof(buf));
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.fillRect(10, 102, 180, 18, TFT_BLACK);
        tft.drawString(buf, 10, 102);
        prevBestLapInt = bestInt;
    }

    // ── Last lap ────────────────────────────────────────────────────────
    int lastInt = (int)(d.lastLap * 1000);
    if (lastInt != prevLastLapInt) {
        formatTime(d.lastLap, buf, sizeof(buf));
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        // Color: green if it's the best, white otherwise
        uint16_t col = (d.lastLap > 0.0f && d.bestLap > 0.0f &&
                        fabsf(d.lastLap - d.bestLap) < 0.001f) ? TFT_GREEN : TFT_WHITE;
        tft.setTextColor(col, TFT_BLACK);
        tft.fillRect(10, 142, 180, 18, TFT_BLACK);
        tft.drawString(buf, 10, 142);
        prevLastLapInt = lastInt;
    }

    // ── Position ────────────────────────────────────────────────────────
    int pos = d.racePosition;
    if (pos != prevPosition) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(3);
        tft.fillRect(10, 182, 100, 30, TFT_BLACK);
        if (pos > 0) {
            uint16_t col = (pos == 1) ? TFT_GOLD : TFT_WHITE;
            tft.setTextColor(col, TFT_BLACK);
            snprintf(buf, sizeof(buf), "P%d", pos);
            tft.drawString(buf, 10, 182);
        }
        prevPosition = pos;
    }

    // ── Speed (right side) ──────────────────────────────────────────────
    float speedFactor = g_settings.useMetric ? 3.6f : 2.23694f;
    int spd = (int)(d.speed * speedFactor);
    if (spd != prevSpeed) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(200, 102, 115, 18, TFT_BLACK);
        snprintf(buf, sizeof(buf), "%d %s", spd, g_settings.useMetric ? "km/h" : "mph");
        tft.drawString(buf, 200, 102);
        prevSpeed = spd;
    }

    // ── Gear (right side) ───────────────────────────────────────────────
    int gear = d.gear;
    if (gear != prevGear) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(3);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(200, 142, 60, 30, TFT_BLACK);
        if (gear == 0) snprintf(buf, sizeof(buf), "R");
        else snprintf(buf, sizeof(buf), "%d", gear);
        tft.drawString(buf, 200, 142);
        prevGear = gear;
    }
}
