#include "display.h"
#include "settings.h"

TFT_eSPI tft = TFT_eSPI();

static uint32_t lastBrightnessUpdate = 0;
static int smoothedLight = 2048;  // start mid-range

void displayInit() {
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    setLED(false, false, false);

    pinMode(LDR_PIN, INPUT);
    analogReadResolution(12);

    tft.init();
    tft.setRotation(1);
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);

    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, g_settings.brightness);
}

void setLED(bool r, bool g, bool b) {
    digitalWrite(LED_R, r ? LOW : HIGH);
    digitalWrite(LED_G, g ? LOW : HIGH);
    digitalWrite(LED_B, b ? LOW : HIGH);
}

void updateAutoBrightness() {
    uint32_t now = millis();
    if (now - lastBrightnessUpdate < 500) return;
    lastBrightnessUpdate = now;

    int raw = analogRead(LDR_PIN);

    // Smooth with exponential moving average
    smoothedLight = (smoothedLight * 7 + raw) / 8;

    // CYD LDR: lower value = more light, higher value = darker
    // Range: ~100 (bright) to ~3800 (dark)
    int bright = map(smoothedLight, 3800, 100, 30, 255);
    if (bright < 30) bright = 30;
    if (bright > 255) bright = 255;

    ledcWrite(0, bright);

    static uint32_t lastDbg = 0;
    if (now - lastDbg > 3000) {
        lastDbg = now;
        Serial.printf("LDR raw=%d smooth=%d bright=%d\n", raw, smoothedLight, bright);
    }
}

// ── RPM bar gradient color ──────────────────────────────────────────────────
// fraction: 0.0 = left edge, 1.0 = right edge of bar
// Returns smooth green → yellow → red gradient
uint16_t rpmBarColor(float fraction) {
    uint8_t r, g, b;

    if (fraction < 0.5f) {
        // Green to Yellow (0.0 - 0.5)
        float t = fraction * 2.0f;
        r = (uint8_t)(t * 255);
        g = 255;
        b = 0;
    } else {
        // Yellow to Red (0.5 - 1.0)
        float t = (fraction - 0.5f) * 2.0f;
        r = 255;
        g = (uint8_t)((1.0f - t) * 255);
        b = 0;
    }

    // Convert to 565
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ── Tire temp color ──────────────────────────────────────────────────────
// Cold (<150°F) = blue, Optimal (180-250°F) = green, Hot (>280°F) = red
void tireTempUpdate(const TelemetryData& d) {
    // no-op, fixed thresholds
}

uint16_t tireTempColor(float tempF) {
    if (tempF < 100.0f) return TFT_BLUE;
    if (tempF < 150.0f) {
        float t = (tempF - 100.0f) / 50.0f;
        uint8_t g = (uint8_t)(t * 255);
        return ((0 & 0xF8) << 8) | ((g & 0xFC) << 3) | (255 >> 3);
    }
    if (tempF < 180.0f) {
        float t = (tempF - 150.0f) / 30.0f;
        uint8_t b = (uint8_t)((1.0f - t) * 255);
        return ((0 & 0xF8) << 8) | ((255 & 0xFC) << 3) | (b >> 3);
    }
    if (tempF <= 250.0f) return TFT_GREEN;
    if (tempF < 280.0f) {
        float t = (tempF - 250.0f) / 30.0f;
        uint8_t r = (uint8_t)(t * 255);
        return ((r & 0xF8) << 8) | ((255 & 0xFC) << 3) | (0 >> 3);
    }
    if (tempF < 320.0f) {
        float t = (tempF - 280.0f) / 40.0f;
        uint8_t g = (uint8_t)((1.0f - t) * 255);
        return ((255 & 0xF8) << 8) | ((g & 0xFC) << 3) | (0 >> 3);
    }
    return TFT_RED;
}

void drawPageDots(int currentPage, int totalPages) {
    const int dotR = 3;
    const int dotSpacing = 14;
    const int dotY = SCREEN_H - 8;
    int totalW = (totalPages - 1) * dotSpacing;
    int startX = (SCREEN_W - totalW) / 2;

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
