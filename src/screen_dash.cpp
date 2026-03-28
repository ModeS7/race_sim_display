#include "screen_dash.h"
#include "settings.h"
#include "seg7.h"
#include "shift_advisor.h"

#define FLASH_INTERVAL_MS 80

// ── Layout (full 240px height) ──────────────────────────────────────────────
#define RPM_BAR_X     10
#define RPM_BAR_Y      6
#define RPM_BAR_W    300
#define RPM_BAR_H     20

#define SHIFT_Y       30
#define SHIFT_H        7
#define SHIFT_SEG_W   32

// RPM number left, Position right
#define RPM_NUM_X     10
#define RPM_NUM_Y     42

#define POS_X        270
#define POS_Y         42

// GEAR big center, Speed left, boost/power/torque right
#define GEAR_X       160
#define GEAR_Y        68
#define SPEED_X       48
#define SPEED_Y       85
#define SPEED_UNIT_Y 135

// Info column (right of gear)
#define INFO_X       245
#define INFO_BOOST_Y  78
#define INFO_POWER_Y  93
#define INFO_TORQUE_Y 108

// Tire temp rectangles (above BRAKE label)
#define TIRE_X       250
#define TIRE_Y       125
#define TIRE_W         6
#define TIRE_H        14
#define TIRE_GAP_X     8
#define TIRE_GAP_Y     4

// Pedal bars
#define PEDAL_Y      175
#define PEDAL_H       24
#define ACCEL_X       10
#define BRAKE_X      165
#define PEDAL_W      145

static uint16_t segmentColor(int seg) {
    if (seg <= 3) return TFT_GREEN;
    if (seg <= 6) return TFT_YELLOW;
    return TFT_RED;
}

static void formatLapTime(float seconds, char* buf, size_t len) {
    if (seconds <= 0.0f) { snprintf(buf, len, "--:--.---"); return; }
    int mins = (int)(seconds / 60.0f);
    float secs = seconds - (mins * 60.0f);
    snprintf(buf, sizeof(buf), "%d:%05.3f", mins, secs);
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
    prevRpm = -1;
    prevBoost = -999.0f;
    prevPower = -1;
    prevTorque = -1;
    prevAccelBar = -1;
    prevBrakeBar = -1;
    prevPosition = -1;
    prevAdvice = -1;
    prevFlashState = false;
    wasFlashing = false;
    flashOn = false;
}

void DashScreen::drawStatic() {
    tft.fillScreen(TFT_BLACK);

    // RPM bar outline
    tft.drawRect(RPM_BAR_X - 1, RPM_BAR_Y - 1,
                 RPM_BAR_W + 2, RPM_BAR_H + 2, TFT_DARKGREY);

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.drawString("RPM", RPM_NUM_X, RPM_NUM_Y);

    // Speed unit
    tft.setTextDatum(MC_DATUM);
    tft.drawString(g_settings.useMetric ? "KM/H" : "MPH", SPEED_X, SPEED_UNIT_Y);

    // Pedal labels and outlines
    tft.setTextDatum(TL_DATUM);
    tft.drawString("ACCEL", ACCEL_X, PEDAL_Y - 10);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("BRAKE", BRAKE_X + PEDAL_W, PEDAL_Y - 10);
    tft.setTextDatum(TL_DATUM);
    tft.drawRect(ACCEL_X - 1, PEDAL_Y - 1, PEDAL_W + 2, PEDAL_H + 2, TFT_DARKGREY);
    tft.drawRect(BRAKE_X - 1, PEDAL_Y - 1, PEDAL_W + 2, PEDAL_H + 2, TFT_DARKGREY);

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

    // ── Shift advisor ────────────────────────────────────────────────────
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
            setLED(flashOn, false, false);    // red LED
        else
            setLED(false, false, flashOn);    // blue LED
        if (flashOn) return;
    } else {
        if (wasFlashing) {
            wasFlashing = false;
            drawStatic();
            resetState();
        }
        prevFlashState = flashOn;
    }

    // ── RPM bar (gradient) ──────────────────────────────────────────────
    int rpmBarW = (int)(rpmFrac * RPM_BAR_W);
    if (rpmBarW != prevRpmBarW) {
        if (rpmBarW > prevRpmBarW && prevRpmBarW >= 0) {
            // Growing — draw new columns with gradient
            for (int x = prevRpmBarW; x < rpmBarW; x++) {
                float frac = (float)x / RPM_BAR_W;
                tft.drawFastVLine(RPM_BAR_X + x, RPM_BAR_Y, RPM_BAR_H, rpmBarColor(frac));
            }
        } else if (rpmBarW < prevRpmBarW) {
            // Shrinking — black out removed region
            tft.fillRect(RPM_BAR_X + rpmBarW, RPM_BAR_Y,
                         prevRpmBarW - rpmBarW, RPM_BAR_H, TFT_BLACK);
        } else {
            // Full redraw
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
        tft.fillRect(RPM_NUM_X, RPM_NUM_Y + 10, 80, 18, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", rpm);
        tft.drawString(buf, RPM_NUM_X, RPM_NUM_Y + 10);
        prevRpm = rpm;
    }

    // ── Position ────────────────────────────────────────────────────────
    int pos = d.racePosition;
    if (pos != prevPosition) {
        tft.fillRect(POS_X - 10, POS_Y, 60, 28, TFT_BLACK);
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
        draw7Seg(tft, GEAR_X - 30, GEAR_Y - 22, 60, 100, 8, gch, gearCol, TFT_BLACK);
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
        tft.fillRect(SPEED_X - 45, SPEED_Y, 90, 35, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", spd);
        tft.drawString(buf, SPEED_X, SPEED_Y + 17);
        prevSpeed = spd;
    }

    // ── Tire temps: [num █]  [█ num] ───────────────────────────────────
    tireTempUpdate(d);
    {
        float temps[4] = { d.tireTempFL, d.tireTempFR, d.tireTempRL, d.tireTempRR };
        // Original rectangle positions
        int rectX0 = TIRE_X;                          // FL, RL
        int rectX1 = TIRE_X + TIRE_W + TIRE_GAP_X;   // FR, RR
        int rectY0 = TIRE_Y;                          // FL, FR
        int rectY1 = TIRE_Y + TIRE_H + TIRE_GAP_Y;   // RL, RR

        tft.setTextSize(1);

        // FL: number left of rectangle [85 █]
        tft.fillRect(rectX0, rectY0, TIRE_W, TIRE_H, tireTempColor(temps[0]));
        char buf[6];
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[0] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[0]);
        tft.setTextDatum(TR_DATUM);
        tft.fillRect(rectX0 - 26, rectY0 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[0]), TFT_BLACK);
        tft.drawString(buf, rectX0 - 2, rectY0 + 3);

        // FR: rectangle left of number [█ 87]
        tft.fillRect(rectX1, rectY0, TIRE_W, TIRE_H, tireTempColor(temps[1]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[1] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[1]);
        tft.setTextDatum(TL_DATUM);
        tft.fillRect(rectX1 + TIRE_W + 2, rectY0 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[1]), TFT_BLACK);
        tft.drawString(buf, rectX1 + TIRE_W + 2, rectY0 + 3);

        // RL: number left of rectangle [82 █]
        tft.fillRect(rectX0, rectY1, TIRE_W, TIRE_H, tireTempColor(temps[2]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[2] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[2]);
        tft.setTextDatum(TR_DATUM);
        tft.fillRect(rectX0 - 26, rectY1 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[2]), TFT_BLACK);
        tft.drawString(buf, rectX0 - 2, rectY1 + 3);

        // RR: rectangle left of number [█ 84]
        tft.fillRect(rectX1, rectY1, TIRE_W, TIRE_H, tireTempColor(temps[3]));
        if (g_settings.useMetric) snprintf(buf, sizeof(buf), "%d", (int)((temps[3] - 32.0f) * 5.0f / 9.0f));
        else snprintf(buf, sizeof(buf), "%d", (int)temps[3]);
        tft.setTextDatum(TL_DATUM);
        tft.fillRect(rectX1 + TIRE_W + 2, rectY1 + 3, 24, 8, TFT_BLACK);
        tft.setTextColor(tireTempColor(temps[3]), TFT_BLACK);
        tft.drawString(buf, rectX1 + TIRE_W + 2, rectY1 + 3);
    }

    // ── Boost / Power / Torque (right of gear, stacked) ─────────────────
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

    // ── Auto brightness ─────────────────────────────────────────────────
    updateAutoBrightness();

    // ── RGB LED ─────────────────────────────────────────────────────────
    if (rpmPct >= g_settings.shiftRedPct)         setLED(true, false, false);
    else if (rpmPct >= g_settings.shiftYellowPct) setLED(true, true, false);
    else if (rpmPct >= g_settings.shiftGreenPct)  setLED(false, true, false);
    else                                           setLED(false, false, false);
}
