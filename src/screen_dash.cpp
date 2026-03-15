#include "screen_dash.h"
#include "settings.h"

#define FLASH_INTERVAL_MS 80

#define DASH_RPM_BAR_Y      4
#define DASH_RPM_BAR_H     20
#define DASH_RPM_BAR_X     10
#define DASH_RPM_BAR_W    300

#define DASH_SHIFT_Y       28
#define DASH_SHIFT_H        8
#define DASH_SHIFT_SEG_W   32

#define DASH_SPEED_Y       70
#define DASH_GEAR_Y        70
#define DASH_SPEED_X       80
#define DASH_GEAR_X       240

#define DASH_UNIT_Y       130

#define DASH_BOOST_Y      155
#define DASH_POWER_Y      175
#define DASH_PEDAL_Y      205
#define DASH_PEDAL_H       20
#define DASH_ACCEL_X       10
#define DASH_BRAKE_X      165
#define DASH_PEDAL_W      145

static uint16_t segmentColor(int seg) {
    if (seg <= 3) return TFT_GREEN;
    if (seg <= 6) return TFT_YELLOW;
    return TFT_RED;
}

void DashScreen::enter() {
    resetState();
    drawStatic();
}

void DashScreen::leave() {
    setLED(false, false, false);
}

void DashScreen::resetState() {
    prevSpeed = -1;
    prevGear = -1;
    prevRpmBarW = -1;
    prevBoost = -999.0f;
    prevPower = -1;
    prevTorque = -1;
    prevAccelBar = -1;
    prevBrakeBar = -1;
    prevFlashState = false;
    wasFlashing = false;
    flashOn = false;
}

void DashScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(DASH_RPM_BAR_X - 1, DASH_RPM_BAR_Y - 1,
                 DASH_RPM_BAR_W + 2, DASH_RPM_BAR_H + 2, TFT_DARKGREY);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString(g_settings.useMetric ? "KM/H" : "MPH", DASH_SPEED_X, DASH_UNIT_Y);
    tft.drawString("GEAR", DASH_GEAR_X, DASH_UNIT_Y);

    tft.setTextDatum(TL_DATUM);
    tft.drawString("ACCEL", DASH_ACCEL_X, DASH_PEDAL_Y - 10);
    tft.drawString("BRAKE", DASH_BRAKE_X, DASH_PEDAL_Y - 10);

    tft.drawRect(DASH_ACCEL_X - 1, DASH_PEDAL_Y - 1,
                 DASH_PEDAL_W + 2, DASH_PEDAL_H + 2, TFT_DARKGREY);
    tft.drawRect(DASH_BRAKE_X - 1, DASH_PEDAL_Y - 1,
                 DASH_PEDAL_W + 2, DASH_PEDAL_H + 2, TFT_DARKGREY);
}

void DashScreen::update(const TelemetryData& d) {
    float rpmFrac = 0.0f;
    if (d.engineMaxRpm > 0.0f) {
        rpmFrac = (d.currentRpm - d.engineIdleRpm) /
                  (d.engineMaxRpm - d.engineIdleRpm);
    }
    if (rpmFrac < 0.0f) rpmFrac = 0.0f;
    if (rpmFrac > 1.0f) rpmFrac = 1.0f;

    float rpmPct = (d.engineMaxRpm > 0.0f) ?
                   (d.currentRpm / d.engineMaxRpm) : 0.0f;

    // ── Flash logic ─────────────────────────────────────────────────────
    bool shouldFlash = (rpmPct >= g_settings.shiftFlashPct);
    uint32_t now = millis();
    if (shouldFlash) {
        if (now - lastFlashToggle >= FLASH_INTERVAL_MS) {
            flashOn = !flashOn;
            lastFlashToggle = now;
        }
    } else {
        flashOn = false;
        lastFlashToggle = now;
    }

    // ── Full-screen shift flash ─────────────────────────────────────────
    if (shouldFlash) {
        if (flashOn != prevFlashState) {
            if (flashOn) {
                tft.fillScreen(TFT_RED);
                tft.setTextDatum(MC_DATUM);
                tft.setTextSize(4);
                tft.setTextColor(TFT_WHITE, TFT_RED);
                tft.drawString("SHIFT!", 160, 110);
            } else {
                drawStatic();
                resetState();
            }
        }
        prevFlashState = flashOn;
        wasFlashing = true;
        setLED(flashOn, false, false);
        return;
    }

    if (wasFlashing) {
        wasFlashing = false;
        drawStatic();
        resetState();
    }
    prevFlashState = flashOn;

    // ── RPM bar (delta draw) ────────────────────────────────────────────
    int rpmBarW = (int)(rpmFrac * DASH_RPM_BAR_W);
    if (rpmBarW != prevRpmBarW) {
        if (rpmBarW > prevRpmBarW && prevRpmBarW >= 0) {
            uint16_t col = TFT_GREEN;
            if (rpmPct >= g_settings.shiftRedPct)        col = TFT_RED;
            else if (rpmPct >= g_settings.shiftYellowPct) col = TFT_YELLOW;
            tft.fillRect(DASH_RPM_BAR_X + prevRpmBarW, DASH_RPM_BAR_Y,
                         rpmBarW - prevRpmBarW, DASH_RPM_BAR_H, col);
        } else if (rpmBarW < prevRpmBarW) {
            tft.fillRect(DASH_RPM_BAR_X + rpmBarW, DASH_RPM_BAR_Y,
                         prevRpmBarW - rpmBarW, DASH_RPM_BAR_H, TFT_BLACK);
        } else {
            uint16_t col = TFT_GREEN;
            if (rpmPct >= g_settings.shiftRedPct)        col = TFT_RED;
            else if (rpmPct >= g_settings.shiftYellowPct) col = TFT_YELLOW;
            tft.fillRect(DASH_RPM_BAR_X, DASH_RPM_BAR_Y,
                         rpmBarW, DASH_RPM_BAR_H, col);
            if (rpmBarW < DASH_RPM_BAR_W) {
                tft.fillRect(DASH_RPM_BAR_X + rpmBarW, DASH_RPM_BAR_Y,
                             DASH_RPM_BAR_W - rpmBarW, DASH_RPM_BAR_H, TFT_BLACK);
            }
        }
        prevRpmBarW = rpmBarW;
    }

    // ── Shift light segments ────────────────────────────────────────────
    for (int seg = 0; seg < 9; seg++) {
        float segThresh;
        if (seg <= 3)      segThresh = g_settings.shiftGreenPct;
        else if (seg <= 6) segThresh = g_settings.shiftYellowPct;
        else               segThresh = g_settings.shiftRedPct;

        int sx = DASH_RPM_BAR_X + seg * (DASH_SHIFT_SEG_W + 2);
        bool lit = (rpmPct >= segThresh);
        uint16_t col = lit ? segmentColor(seg) : TFT_DARKGREY;
        tft.fillRect(sx, DASH_SHIFT_Y, DASH_SHIFT_SEG_W, DASH_SHIFT_H, col);
    }

    // ── Speed ───────────────────────────────────────────────────────────
    float speedFactor = g_settings.useMetric ? 3.6f : 2.23694f;
    int spd = (int)(d.speed * speedFactor);
    if (spd != prevSpeed) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(6);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(DASH_SPEED_X - 55, DASH_SPEED_Y, 110, 50, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%3d", spd);
        tft.drawString(buf, DASH_SPEED_X, DASH_SPEED_Y + 25);
        prevSpeed = spd;
    }

    // ── Gear ────────────────────────────────────────────────────────────
    int gear = d.gear;
    if (gear != prevGear) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(6);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(DASH_GEAR_X - 25, DASH_GEAR_Y, 50, 50, TFT_BLACK);
        char buf[4];
        if (gear == 0) snprintf(buf, sizeof(buf), "R");
        else snprintf(buf, sizeof(buf), "%d", gear);
        tft.drawString(buf, DASH_GEAR_X, DASH_GEAR_Y + 25);
        prevGear = gear;
    }

    // ── Boost ───────────────────────────────────────────────────────────
    float boostVal = g_settings.useMetric ? (d.boost * 1.01325f) : (d.boost * 14.696f);
    int boostDisp = (int)(boostVal * 100);
    int prevBoostDisp = (int)(prevBoost * 100);
    if (boostDisp != prevBoostDisp) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.fillRect(0, DASH_BOOST_Y, 160, 12, TFT_BLACK);
        if (boostVal > 0.01f) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            char buf[24];
            if (g_settings.useMetric)
                snprintf(buf, sizeof(buf), "BOOST: %.2f bar", boostVal);
            else
                snprintf(buf, sizeof(buf), "BOOST: %.1f PSI", boostVal);
            tft.drawString(buf, 10, DASH_BOOST_Y);
        }
        prevBoost = boostVal;
    }

    // ── Power ───────────────────────────────────────────────────────────
    int pw;
    if (g_settings.useMetric) pw = (int)(d.power / 1000.0f);
    else pw = (int)(d.power / 745.7f);
    if (pw != prevPower) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.fillRect(160, DASH_POWER_Y, 160, 12, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        char buf[24];
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "kW: %d", pw);
        else snprintf(buf, sizeof(buf), "HP: %d", pw);
        tft.drawString(buf, 170, DASH_POWER_Y);
        prevPower = pw;
    }

    // ── Torque ──────────────────────────────────────────────────────────
    int tq = (int)d.torque;
    if (tq != prevTorque) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.fillRect(0, DASH_POWER_Y, 160, 12, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        char buf[24];
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "TQ: %d Nm", tq);
        else snprintf(buf, sizeof(buf), "TQ: %d lb-ft", (int)(tq * 0.7376f));
        tft.drawString(buf, 10, DASH_POWER_Y);
        prevTorque = tq;
    }

    // ── Accel pedal bar ─────────────────────────────────────────────────
    int accelW = (d.accel * DASH_PEDAL_W) / 255;
    if (accelW != prevAccelBar) {
        if (accelW < prevAccelBar && prevAccelBar > 0)
            tft.fillRect(DASH_ACCEL_X + accelW, DASH_PEDAL_Y,
                         prevAccelBar - accelW, DASH_PEDAL_H, TFT_BLACK);
        if (accelW > 0)
            tft.fillRect(DASH_ACCEL_X, DASH_PEDAL_Y, accelW, DASH_PEDAL_H, TFT_GREEN);
        prevAccelBar = accelW;
    }

    // ── Brake pedal bar ─────────────────────────────────────────────────
    int brakeW = (d.brake * DASH_PEDAL_W) / 255;
    if (brakeW != prevBrakeBar) {
        if (brakeW < prevBrakeBar && prevBrakeBar > 0)
            tft.fillRect(DASH_BRAKE_X + brakeW, DASH_PEDAL_Y,
                         prevBrakeBar - brakeW, DASH_PEDAL_H, TFT_BLACK);
        if (brakeW > 0)
            tft.fillRect(DASH_BRAKE_X, DASH_PEDAL_Y, brakeW, DASH_PEDAL_H, TFT_RED);
        prevBrakeBar = brakeW;
    }

    // ── RGB LED ─────────────────────────────────────────────────────────
    if (rpmPct >= g_settings.shiftRedPct)         setLED(true, false, false);
    else if (rpmPct >= g_settings.shiftYellowPct) setLED(true, true, false);
    else if (rpmPct >= g_settings.shiftGreenPct)  setLED(false, true, false);
    else                                           setLED(false, false, false);
}
