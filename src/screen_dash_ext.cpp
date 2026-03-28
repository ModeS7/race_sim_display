#include "screen_dash_ext.h"
#include "settings.h"
#include "seg7.h"
#include "shift_advisor.h"

#define FLASH_INTERVAL_MS 80

// ── Layout (full 240px height) ──────────────────────────────────────────────
#define RPM_BAR_X     10
#define RPM_BAR_Y      6
#define RPM_BAR_W    300
#define RPM_BAR_H     18

#define SHIFT_Y       28
#define SHIFT_H        6
#define SHIFT_SEG_W   32

#define RPM_NUM_X     10
#define RPM_NUM_Y     38
#define POS_X        270
#define POS_Y         38

#define GEAR_X       160
#define GEAR_Y        58
#define SPEED_X       45
#define SPEED_Y       72

#define TIRE_X       250
#define TIRE_Y        95
#define TIRE_W         6
#define TIRE_H        14
#define TIRE_GAP_X     8
#define TIRE_GAP_Y     4

#define INFO_X       245
#define INFO_BOOST_Y  65
#define INFO_POWER_Y  78
#define INFO_TORQUE_Y 91

#define PEDAL_Y      140
#define PEDAL_H       20
#define ACCEL_X       10
#define BRAKE_X      165
#define PEDAL_W      145

// Lap section — best & last side by side
#define LAP_CUR_Y    175
#define LAP_BEST_X    10
#define LAP_LAST_X   170

static uint16_t segmentColor(int seg) {
    if (seg <= 3) return TFT_GREEN;
    if (seg <= 6) return TFT_YELLOW;
    return TFT_RED;
}

static void formatLapTime(float seconds, char* buf, size_t len) {
    if (seconds <= 0.0f) { snprintf(buf, len, "--:--.---"); return; }
    int mins = (int)(seconds / 60.0f);
    float secs = seconds - (mins * 60.0f);
    snprintf(buf, len, "%d:%05.3f", mins, secs);
}

void DashExtScreen::enter() {
    resetState();
    drawStatic();
}

void DashExtScreen::leave() {
    setLED(false, false, false);
}

void DashExtScreen::resetState() {
    prevSpeed = -1;
    prevGear = -1;
    prevRpmBarW = -1;
    prevRpm = -1;
    prevBoost = -999.0f;
    prevPower = -1;
    prevTorque = -1;
    prevAccelBar = -1;
    prevBrakeBar = -1;
    prevPosition = -1;
    prevCurrentLap = -1;
    prevBestLap = -1;
    prevLastLap = -1;
    prevAdvice = -1;
    prevFlashState = false;
    wasFlashing = false;
    flashOn = false;
}

void DashExtScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    tft.drawRect(RPM_BAR_X - 1, RPM_BAR_Y - 1,
                 RPM_BAR_W + 2, RPM_BAR_H + 2, TFT_DARKGREY);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.drawString("RPM", RPM_NUM_X, RPM_NUM_Y);

    tft.setTextDatum(MC_DATUM);
    tft.drawString(g_settings.useMetric ? "KM/H" : "MPH", SPEED_X, SPEED_Y + 38);

    tft.setTextDatum(TL_DATUM);
    tft.drawString("THR", ACCEL_X, PEDAL_Y - 9);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("BRK", BRAKE_X + PEDAL_W, PEDAL_Y - 9);
    tft.setTextDatum(TL_DATUM);
    tft.drawRect(ACCEL_X - 1, PEDAL_Y - 1, PEDAL_W + 2, PEDAL_H + 2, TFT_DARKGREY);
    tft.drawRect(BRAKE_X - 1, PEDAL_Y - 1, PEDAL_W + 2, PEDAL_H + 2, TFT_DARKGREY);

    // Lap section
    tft.drawFastHLine(10, LAP_CUR_Y - 4, 300, 0x2104);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("BEST", LAP_BEST_X, LAP_CUR_Y);
    tft.drawString("LAST", LAP_LAST_X, LAP_CUR_Y);
}

void DashExtScreen::update(const TelemetryData& d) {
    float rpmFrac = 0.0f;
    if (d.engineMaxRpm > 0.0f) {
        rpmFrac = (d.currentRpm - d.engineIdleRpm) /
                  (d.engineMaxRpm - d.engineIdleRpm);
    }
    if (rpmFrac < 0.0f) rpmFrac = 0.0f;
    if (rpmFrac > 1.0f) rpmFrac = 1.0f;

    float rpmPct = (d.engineMaxRpm > 0.0f) ?
                   (d.currentRpm / d.engineMaxRpm) : 0.0f;

    g_shiftAdvisor.update(d);
    ShiftAdvice advice = g_shiftAdvisor.getAdvice();
    bool shouldFlash = (advice == SHIFT_UPSHIFT || advice == SHIFT_DOWNSHIFT);
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
                if (advice == SHIFT_UPSHIFT) {
                    tft.fillScreen(TFT_RED);
                    tft.setTextDatum(MC_DATUM);
                    tft.setTextSize(4);
                    tft.setTextColor(TFT_WHITE, TFT_RED);
                    tft.drawString("SHIFT!", 160, 110);
                } else {
                    tft.fillScreen(TFT_BLUE);
                    tft.setTextDatum(MC_DATUM);
                    tft.setTextSize(4);
                    tft.setTextColor(TFT_WHITE, TFT_BLUE);
                    tft.drawString("DOWN!", 160, 110);
                }
            } else {
                drawStatic();
                resetState();
            }
        }
        prevFlashState = flashOn;
        wasFlashing = true;
        if (advice == SHIFT_UPSHIFT)
            setLED(flashOn, false, false);
        else
            setLED(false, false, flashOn);
        if (flashOn) return;
    } else {
        if (wasFlashing) {
            wasFlashing = false;
            drawStatic();
            resetState();
        }
        prevFlashState = flashOn;
    }

    // ── RPM bar ─────────────────────────────────────────────────────────
    int rpmBarW = (int)(rpmFrac * RPM_BAR_W);
    if (rpmBarW != prevRpmBarW) {
        if (rpmBarW > prevRpmBarW && prevRpmBarW >= 0) {
            for (int x = prevRpmBarW; x < rpmBarW; x++) {
                float frac = (float)x / RPM_BAR_W;
                tft.drawFastVLine(RPM_BAR_X + x, RPM_BAR_Y, RPM_BAR_H, rpmBarColor(frac));
            }
        } else if (rpmBarW < prevRpmBarW) {
            tft.fillRect(RPM_BAR_X + rpmBarW, RPM_BAR_Y,
                         prevRpmBarW - rpmBarW, RPM_BAR_H, TFT_BLACK);
        } else {
            for (int x = 0; x < rpmBarW; x++) {
                float frac = (float)x / RPM_BAR_W;
                tft.drawFastVLine(RPM_BAR_X + x, RPM_BAR_Y, RPM_BAR_H, rpmBarColor(frac));
            }
            if (rpmBarW < RPM_BAR_W)
                tft.fillRect(RPM_BAR_X + rpmBarW, RPM_BAR_Y,
                             RPM_BAR_W - rpmBarW, RPM_BAR_H, TFT_BLACK);
        }
        prevRpmBarW = rpmBarW;
    }

    // ── Shift lights ────────────────────────────────────────────────────
    for (int seg = 0; seg < 9; seg++) {
        float segThresh;
        if (seg <= 3)      segThresh = g_settings.shiftGreenPct;
        else if (seg <= 6) segThresh = g_settings.shiftYellowPct;
        else               segThresh = g_settings.shiftRedPct;
        int sx = RPM_BAR_X + seg * (SHIFT_SEG_W + 2);
        bool lit = (rpmPct >= segThresh);
        tft.fillRect(sx, SHIFT_Y, SHIFT_SEG_W, SHIFT_H,
                     lit ? segmentColor(seg) : TFT_DARKGREY);
    }

    // ── RPM number ──────────────────────────────────────────────────────
    int rpm = ((int)d.currentRpm / 100) * 100;
    if (rpm != prevRpm) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(RPM_NUM_X, RPM_NUM_Y + 10, 80, 16, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", rpm);
        tft.drawString(buf, RPM_NUM_X, RPM_NUM_Y + 10);
        prevRpm = rpm;
    }

    // ── Position ────────────────────────────────────────────────────────
    int pos = d.racePosition;
    if (pos != prevPosition) {
        tft.fillRect(POS_X - 10, POS_Y, 60, 24, TFT_BLACK);
        if (pos > 0 && pos <= 24) {
            tft.setTextDatum(TR_DATUM);
            tft.setTextSize(3);
            uint16_t col = (pos == 1) ? TFT_GOLD : (pos <= 3) ? TFT_GREEN : TFT_WHITE;
            tft.setTextColor(col, TFT_BLACK);
            char buf[6];
            snprintf(buf, sizeof(buf), "P%d", pos);
            tft.drawString(buf, 310, POS_Y);
        }
        prevPosition = pos;
    }

    // ── GEAR (7-segment, color changes with shift advice) ──────────────
    int gear = d.gear;
    int curAdvice = (int)g_shiftAdvisor.getAdvice();
    if (gear != prevGear || curAdvice != prevAdvice) {
        uint16_t gearCol = TFT_WHITE;
        if (curAdvice == SHIFT_DOWNSHIFT) gearCol = TFT_CYAN;
        else if (curAdvice == SHIFT_UPSHIFT) gearCol = TFT_RED;
        char gch = (gear == 0) ? 'R' : ('0' + gear);
        draw7Seg(tft, GEAR_X - 25, GEAR_Y - 18, 50, 80, 7, gch, gearCol, TFT_BLACK);
        prevGear = gear;
        prevAdvice = curAdvice;
    }

    // ── Speed ───────────────────────────────────────────────────────────
    float speedFactor = g_settings.useMetric ? 3.6f : 2.23694f;
    int spd = (int)(d.speed * speedFactor);
    if (spd != prevSpeed) {
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(4);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.fillRect(SPEED_X - 45, SPEED_Y, 90, 32, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", spd);
        tft.drawString(buf, SPEED_X, SPEED_Y + 16);
        prevSpeed = spd;
    }

    // ── Tire temps: [num █]  [█ num] ───────────────────────────────────
    tireTempUpdate(d);
    {
        float temps[4] = { d.tireTempFL, d.tireTempFR, d.tireTempRL, d.tireTempRR };
        int rectX0 = TIRE_X;
        int rectX1 = TIRE_X + TIRE_W + TIRE_GAP_X;
        int rectY0 = TIRE_Y;
        int rectY1 = TIRE_Y + TIRE_H + TIRE_GAP_Y;
        char buf[6];

        tft.setTextSize(1);

        // FL: [num █]
        tft.fillRect(rectX0, rectY0, TIRE_W, TIRE_H, tireTempColor(temps[0]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[0] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[0]);
        tft.setTextDatum(TR_DATUM);
        tft.fillRect(rectX0 - 26, rectY0 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[0]), TFT_BLACK);
        tft.drawString(buf, rectX0 - 2, rectY0 + 3);

        // FR: [█ num]
        tft.fillRect(rectX1, rectY0, TIRE_W, TIRE_H, tireTempColor(temps[1]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[1] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[1]);
        tft.setTextDatum(TL_DATUM);
        tft.fillRect(rectX1 + TIRE_W + 2, rectY0 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[1]), TFT_BLACK);
        tft.drawString(buf, rectX1 + TIRE_W + 2, rectY0 + 3);

        // RL: [num █]
        tft.fillRect(rectX0, rectY1, TIRE_W, TIRE_H, tireTempColor(temps[2]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[2] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[2]);
        tft.setTextDatum(TR_DATUM);
        tft.fillRect(rectX0 - 26, rectY1 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[2]), TFT_BLACK);
        tft.drawString(buf, rectX0 - 2, rectY1 + 3);

        // RR: [█ num]
        tft.fillRect(rectX1, rectY1, TIRE_W, TIRE_H, tireTempColor(temps[3]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[3] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[3]);
        tft.setTextDatum(TL_DATUM);
        tft.fillRect(rectX1 + TIRE_W + 2, rectY1 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[3]), TFT_BLACK);
        tft.drawString(buf, rectX1 + TIRE_W + 2, rectY1 + 3);
    }

    // ── Info: Boost / Power / Torque (stacked right of gear) ──────────────
    char buf[24];
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);

    float boostVal = g_settings.useMetric ? (d.boost * 1.01325f) : (d.boost * 14.696f);
    int boostDisp = (int)(boostVal * 100);
    int prevBoostDisp = (int)(prevBoost * 100);
    if (boostDisp != prevBoostDisp) {
        tft.fillRect(INFO_X, INFO_BOOST_Y, 75, 10, TFT_BLACK);
        if (boostVal > 0.01f) {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%.2f bar", boostVal);
            else snprintf(buf, sizeof(buf), "%.1f PSI", boostVal);
            tft.drawString(buf, INFO_X, INFO_BOOST_Y);
        }
        prevBoost = boostVal;
    }

    int pw = g_settings.useMetric ? (int)(d.power / 1000.0f) : (int)(d.power / 745.7f);
    if (pw != prevPower) {
        tft.fillRect(INFO_X, INFO_POWER_Y, 75, 10, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d kW", pw);
        else snprintf(buf, sizeof(buf), "%d HP", pw);
        tft.drawString(buf, INFO_X, INFO_POWER_Y);
        prevPower = pw;
    }

    int tq = (int)d.torque;
    if (tq != prevTorque) {
        tft.fillRect(INFO_X, INFO_TORQUE_Y, 75, 10, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d Nm", tq);
        else snprintf(buf, sizeof(buf), "%d ft-lb", (int)(tq * 0.7376f));
        tft.drawString(buf, INFO_X, INFO_TORQUE_Y);
        prevTorque = tq;
    }

    // ── Pedal bars ──────────────────────────────────────────────────────
    int accelW = (d.accel * PEDAL_W) / 255;
    if (accelW != prevAccelBar) {
        if (accelW < prevAccelBar && prevAccelBar > 0)
            tft.fillRect(ACCEL_X + accelW, PEDAL_Y,
                         prevAccelBar - accelW, PEDAL_H, TFT_BLACK);
        if (accelW > 0)
            tft.fillRect(ACCEL_X, PEDAL_Y, accelW, PEDAL_H, TFT_GREEN);
        prevAccelBar = accelW;
    }

    int brakeW = (d.brake * PEDAL_W) / 255;
    if (brakeW != prevBrakeBar) {
        if (brakeW < prevBrakeBar && prevBrakeBar > 0)
            tft.fillRect(BRAKE_X + PEDAL_W - prevBrakeBar, PEDAL_Y,
                         prevBrakeBar - brakeW, PEDAL_H, TFT_BLACK);
        if (brakeW > 0)
            tft.fillRect(BRAKE_X + PEDAL_W - brakeW, PEDAL_Y, brakeW, PEDAL_H, TFT_RED);
        prevBrakeBar = brakeW;
    }

    // ── Best/last lap times ────────────────────────────────────────────
    if (d.lapNumber > 0) {
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);

        int bestLapInt = (int)(d.bestLap * 1000);
        if (bestLapInt != prevBestLap) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.fillRect(LAP_BEST_X, LAP_CUR_Y + 10, 150, 18, TFT_BLACK);
            formatLapTime(d.bestLap, buf, sizeof(buf));
            tft.drawString(buf, LAP_BEST_X, LAP_CUR_Y + 10);
            prevBestLap = bestLapInt;
        }

        int lastLapInt = (int)(d.lastLap * 1000);
        if (lastLapInt != prevLastLap) {
            uint16_t col = (d.lastLap > 0.0f && d.bestLap > 0.0f &&
                            fabsf(d.lastLap - d.bestLap) < 0.001f) ? TFT_GREEN : TFT_YELLOW;
            tft.setTextColor(col, TFT_BLACK);
            tft.fillRect(LAP_LAST_X, LAP_CUR_Y + 10, 150, 18, TFT_BLACK);
            formatLapTime(d.lastLap, buf, sizeof(buf));
            tft.drawString(buf, LAP_LAST_X, LAP_CUR_Y + 10);
            prevLastLap = lastLapInt;
        }
    } else if (prevBestLap != 0) {
        tft.fillRect(LAP_BEST_X, LAP_CUR_Y + 10, 150, 18, TFT_BLACK);
        tft.fillRect(LAP_LAST_X, LAP_CUR_Y + 10, 150, 18, TFT_BLACK);
        prevBestLap = 0;
        prevLastLap = 0;
    }

    // ── Auto brightness ─────────────────────────────────────────────────
    updateAutoBrightness();

    // ── RGB LED ─────────────────────────────────────────────────────────
    if (rpmPct >= g_settings.shiftRedPct)         setLED(true, false, false);
    else if (rpmPct >= g_settings.shiftYellowPct) setLED(true, true, false);
    else if (rpmPct >= g_settings.shiftGreenPct)  setLED(false, true, false);
    else                                           setLED(false, false, false);
}
