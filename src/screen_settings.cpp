#include "screen_settings.h"
#include "settings.h"
#include <WiFi.h>

// Row indices
#define ROW_UNITS     0
#define ROW_BRIGHT    1
#define ROW_SHIFTMODE 2
#define ROW_SHIFT     3
#define ROW_WIFI      4
#define ROW_SAVE      5

void SettingsScreen::enter() {
    drawStatic();
}

void SettingsScreen::leave() {}

void SettingsScreen::update(const TelemetryData& data) {}

void SettingsScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("SETTINGS", 160, 12);

    for (int i = 0; i < NUM_ROWS; i++) {
        drawRow(i);
    }
}

void SettingsScreen::drawRow(int row) {
    int y = ROW_Y0 + row * ROW_H;
    char buf[32];

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);

    tft.fillRect(0, y, 320, ROW_H - 2, TFT_BLACK);

    switch (row) {
    case ROW_UNITS:
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("Units:", 10, y + 6);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(g_settings.useMetric ? "METRIC" : "IMPERIAL", 160, y + 6);
        break;

    case ROW_BRIGHT:
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("Bright:", 10, y + 6);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        snprintf(buf, sizeof(buf), "%d%%", (g_settings.brightness * 100) / 255);
        tft.drawString(buf, 220, y + 6);
        {
            int barW = (g_settings.brightness * 50) / 255;
            tft.fillRect(160, y + 8, 50, 12, TFT_DARKGREY);
            tft.fillRect(160, y + 8, barW, 12, TFT_CYAN);
        }
        break;

    case ROW_SHIFTMODE:
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("Shift:", 10, y + 6);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        if (g_settings.shiftMode == SHIFT_MODE_PERCENT)
            tft.drawString("PERCENT", 160, y + 6);
        else
            tft.drawString("ADAPTIVE", 160, y + 6);
        break;

    case ROW_SHIFT:
        if (g_settings.shiftMode == SHIFT_MODE_PERCENT) {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString("Shift @:", 10, y + 6);
            tft.setTextColor(TFT_RED, TFT_BLACK);
            snprintf(buf, sizeof(buf), "%d%%", (int)(g_settings.shiftFlashPct * 100));
            tft.drawString(buf, 160, y + 6);
        } else {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString("Mode:", 10, y + 6);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString("AUTO-LEARN", 160, y + 6);
        }
        break;

    case ROW_WIFI:
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("WiFi:", 10, y + 6);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        tft.drawString(g_settings.wifiSSID, 160, y + 4);
        {
            String ip = WiFi.localIP().toString();
            tft.drawString(ip.c_str(), 160, y + 16);
        }
        break;

    case ROW_SAVE:
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.fillRoundRect(100, y + 2, 120, ROW_H - 6, 4, TFT_GREEN);
        tft.drawString("SAVE", 160, y + ROW_H / 2 - 1);
        tft.setTextDatum(TL_DATUM);
        break;
    }
}

bool SettingsScreen::handleTouch(int x, int y) {
    int row = (y - ROW_Y0) / ROW_H;
    if (row < 0 || row >= NUM_ROWS) return false;

    switch (row) {
    case ROW_UNITS:
        g_settings.useMetric = !g_settings.useMetric;
        drawRow(ROW_UNITS);
        return true;

    case ROW_BRIGHT: {
        int step = 51;
        if (x < 160) {
            if (g_settings.brightness > step) g_settings.brightness -= step;
            else g_settings.brightness = 25;
        } else {
            if (g_settings.brightness + step <= 255) g_settings.brightness += step;
            else g_settings.brightness = 255;
        }
        ledcWrite(0, g_settings.brightness);
        drawRow(ROW_BRIGHT);
        return true;
    }

    case ROW_SHIFTMODE:
        g_settings.shiftMode = (g_settings.shiftMode == SHIFT_MODE_PERCENT)
            ? SHIFT_MODE_ADAPTIVE : SHIFT_MODE_PERCENT;
        drawRow(ROW_SHIFTMODE);
        drawRow(ROW_SHIFT);  // update the row below too
        return true;

    case ROW_SHIFT: {
        if (g_settings.shiftMode == SHIFT_MODE_PERCENT) {
            static const float presets[] = {0.85f, 0.88f, 0.90f, 0.92f, 0.95f, 0.97f};
            static const int nPresets = sizeof(presets) / sizeof(presets[0]);
            int cur = 0;
            for (int i = 0; i < nPresets; i++) {
                if (fabsf(g_settings.shiftFlashPct - presets[i]) < 0.005f) { cur = i; break; }
            }
            cur = (cur + 1) % nPresets;
            g_settings.shiftFlashPct = presets[cur];
            drawRow(ROW_SHIFT);
        }
        return true;
    }

    case ROW_SAVE:
        settingsSave();
        tft.fillRoundRect(100, ROW_Y0 + ROW_SAVE * ROW_H + 2, 120, ROW_H - 6, 4, TFT_WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        tft.drawString("SAVED!", 160, ROW_Y0 + ROW_SAVE * ROW_H + ROW_H / 2 - 1);
        delay(500);
        drawRow(ROW_SAVE);
        return true;

    default:
        return false;
    }
}
