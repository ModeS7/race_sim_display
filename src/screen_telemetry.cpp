#include "screen_telemetry.h"
#include "settings.h"

void TelemetryScreen::enter() {
    prevDotX = -1;
    prevDotY = -1;
    prevSpeed = -1;
    prevRpm = -1;
    prevPower = -1;
    prevTorque = -1;
    drawStatic();
}

void TelemetryScreen::leave() {}

void TelemetryScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    // G-force title
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("G-FORCE", GF_CX, 30);

    // G-force circle outline
    tft.drawCircle(GF_CX, GF_CY, GF_R, TFT_DARKGREY);
    tft.drawCircle(GF_CX, GF_CY, GF_R / 2, 0x2104);  // inner 0.5G ring (dark gray)

    // Crosshairs
    tft.drawFastHLine(GF_CX - GF_R, GF_CY, GF_R * 2, 0x2104);
    tft.drawFastVLine(GF_CX, GF_CY - GF_R, GF_R * 2, 0x2104);

    // Right side labels
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("LAT G", 175, 40);
    tft.drawString("LON G", 175, 70);
    tft.drawString("SPEED", 175, 110);
    tft.drawString("RPM", 175, 140);
    tft.drawString(g_settings.useMetric ? "kW" : "HP", 175, 170);
    tft.drawString("TORQUE", 175, 200);
}

void TelemetryScreen::update(const TelemetryData& d) {
    // ── G-force dot ─────────────────────────────────────────────────────
    // Convert m/s^2 to G and map to pixel offset
    // 1G = GF_R pixels, clamp to circle
    float gLat = d.accelX / 9.81f;   // lateral (left/right)
    float gLon = -d.accelZ / 9.81f;  // longitudinal (braking positive = up on screen)

    int dotX = GF_CX + (int)(gLat * GF_R);
    int dotY = GF_CY - (int)(gLon * GF_R);

    // Clamp to circle
    float dx = dotX - GF_CX;
    float dy = dotY - GF_CY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > GF_R - DOT_R) {
        float scale = (GF_R - DOT_R) / dist;
        dotX = GF_CX + (int)(dx * scale);
        dotY = GF_CY + (int)(dy * scale);
    }

    if (dotX != prevDotX || dotY != prevDotY) {
        // Erase old dot
        if (prevDotX >= 0) {
            tft.fillCircle(prevDotX, prevDotY, DOT_R, TFT_BLACK);
            // Redraw crosshairs/rings where dot was
            if (abs(prevDotY - GF_CY) <= 1)
                tft.drawFastHLine(prevDotX - DOT_R - 1, GF_CY, DOT_R * 2 + 2, 0x2104);
            if (abs(prevDotX - GF_CX) <= 1)
                tft.drawFastVLine(GF_CX, prevDotY - DOT_R - 1, DOT_R * 2 + 2, 0x2104);
        }
        // Draw new dot
        tft.fillCircle(dotX, dotY, DOT_R, TFT_YELLOW);
        prevDotX = dotX;
        prevDotY = dotY;
    }

    // ── Numeric readouts (right side) ───────────────────────────────────
    char buf[16];

    // Lateral G
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%+.2f", gLat);
    tft.fillRect(175, 50, 140, 16, TFT_BLACK);
    tft.drawString(buf, 175, 50);

    // Longitudinal G
    snprintf(buf, sizeof(buf), "%+.2f", gLon);
    tft.fillRect(175, 80, 140, 16, TFT_BLACK);
    tft.drawString(buf, 175, 80);

    // Speed
    float speedFactor = g_settings.useMetric ? 3.6f : 2.23694f;
    int spd = (int)(d.speed * speedFactor);
    if (spd != prevSpeed) {
        snprintf(buf, sizeof(buf), "%d", spd);
        tft.fillRect(175, 120, 140, 16, TFT_BLACK);
        tft.drawString(buf, 175, 120);
        prevSpeed = spd;
    }

    // RPM
    int rpm = (int)d.currentRpm;
    if (rpm != prevRpm) {
        snprintf(buf, sizeof(buf), "%d", rpm);
        tft.fillRect(175, 150, 140, 16, TFT_BLACK);
        tft.drawString(buf, 175, 150);
        prevRpm = rpm;
    }

    // Power
    int pw = g_settings.useMetric ? (int)(d.power / 1000.0f) : (int)(d.power / 745.7f);
    if (pw != prevPower) {
        snprintf(buf, sizeof(buf), "%d", pw);
        tft.fillRect(175, 180, 140, 16, TFT_BLACK);
        tft.drawString(buf, 175, 180);
        prevPower = pw;
    }

    // Torque
    int tq = g_settings.useMetric ? (int)d.torque : (int)(d.torque * 0.7376f);
    if (tq != prevTorque) {
        snprintf(buf, sizeof(buf), "%d", tq);
        tft.fillRect(175, 210, 140, 16, TFT_BLACK);
        tft.drawString(buf, 175, 210);
        prevTorque = tq;
    }
}
