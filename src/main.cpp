#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "telemetry.h"
#include "display.h"
#include "touch.h"
#include "settings.h"
#include "screen_dash.h"
#include "screen_dash_ext.h"
#include "screen_laps.h"
#include "screen_settings.h"

// ── Screens ─────────────────────────────────────────────────────────────────
DashScreen      screenDash;
DashExtScreen   screenDashExt;
LapsScreen      screenLaps;
SettingsScreen  screenSettings;

ScreenBase* screens[] = { &screenDash, &screenDashExt, &screenLaps, &screenSettings };
const int NUM_SCREENS = 4;
int currentScreenIdx = 0;

// ── App state ───────────────────────────────────────────────────────────────
enum AppState { STATE_CONNECTING, STATE_WAITING, STATE_ACTIVE };
AppState appState = STATE_CONNECTING;

uint32_t lastDrawTime = 0;
uint32_t lastRaceOnTime = 0;
uint32_t waitingDotTimer = 0;
int waitingDotIdx = 0;

#define FRAME_INTERVAL_MS 33
#define RACE_OFF_TIMEOUT_MS 20000

// ── Helpers ─────────────────────────────────────────────────────────────────

void switchScreen(int newIdx) {
    screens[currentScreenIdx]->leave();
    currentScreenIdx = newIdx;
    screens[currentScreenIdx]->enter();
    drawPageDots(currentScreenIdx, NUM_SCREENS);
}

// ── Concept 4: Race Stripe — Boot/Connecting Screen ─────────────────────────
void drawConnectingScreen() {
    tft.fillScreen(TFT_BLACK);

    // Top and bottom red bars
    tft.fillRect(0, 0, 320, 6, TFT_RED);
    tft.fillRect(0, 234, 320, 6, TFT_RED);

    // Diagonal stripes
    uint16_t stripeCol = 0x1082;
    for (int i = 0; i < 3; i++) {
        int sx = 60 + i * 100;
        tft.fillTriangle(sx, 20, sx + 30, 20, sx + 10, 90, stripeCol);
        tft.fillTriangle(sx + 30, 20, sx + 10, 90, sx + 40, 90, stripeCol);
    }
    for (int i = 0; i < 3; i++) {
        int sx = 60 + i * 100;
        tft.fillTriangle(sx + 10, 145, sx + 40, 145, sx + 30, 215, stripeCol);
        tft.fillTriangle(sx + 40, 145, sx + 30, 215, sx + 60, 215, stripeCol);
    }

    // Title
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("FORZA", 160, 105);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("DASH", 160, 132);

    // Status
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Connecting WiFi...", 160, 220);
}

void drawWifiFailed() {
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 320, 6, TFT_RED);
    tft.fillRect(0, 234, 320, 6, TFT_RED);

    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("WiFi FAILED", 160, 100);
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Check SSID/password and reboot", 160, 140);
}

// ── Concept 1: Pit Lane — Waiting for Telemetry Screen ──────────────────────
void drawWaitingScreen() {
    tft.fillScreen(TFT_BLACK);

    // Cyan horizontal rules
    tft.drawFastHLine(20, 50, 280, TFT_CYAN);
    tft.drawFastHLine(20, 52, 280, TFT_CYAN);

    // Title
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("FORZA DASH", 160, 75);

    tft.drawFastHLine(20, 98, 280, TFT_CYAN);
    tft.drawFastHLine(20, 100, 280, TFT_CYAN);

    // Scanning dots (initial state)
    for (int i = 0; i < 4; i++) {
        int cx = 130 + i * 20;
        tft.drawCircle(cx, 120, 4, TFT_CYAN);
    }
    waitingDotIdx = 0;
    waitingDotTimer = millis();

    // Subtitle
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Waiting for telemetry...", 160, 140);

    // IP box
    tft.drawRoundRect(60, 155, 200, 24, 4, TFT_CYAN);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    String ipStr = WiFi.localIP().toString() + ":" + String(g_settings.udpPort);
    tft.drawString(ipStr, 160, 167);

    // Instructions
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Settings > HUD & Gameplay > Data Out", 160, 195);
}

void updateWaitingDots() {
    uint32_t now = millis();
    if (now - waitingDotTimer < 300) return;
    waitingDotTimer = now;

    // Clear previous dot
    int prevCx = 130 + waitingDotIdx * 20;
    tft.fillCircle(prevCx, 120, 4, TFT_BLACK);
    tft.drawCircle(prevCx, 120, 4, TFT_CYAN);

    // Advance
    waitingDotIdx = (waitingDotIdx + 1) % 4;

    // Fill current dot
    int cx = 130 + waitingDotIdx * 20;
    tft.fillCircle(cx, 120, 4, TFT_CYAN);
}

// ── Concept 3: Grid Menu — Screen Selection ─────────────────────────────────
#define MENU_BW 145
#define MENU_BH 90
#define MENU_X1 8
#define MENU_X2 167
#define MENU_Y1 28
#define MENU_Y2 124

void drawGridMenu() {
    tft.fillScreen(TFT_BLACK);

    // Header bar
    tft.fillRect(0, 0, 320, 22, 0x2104);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, 0x2104);
    tft.drawString("FORZA DASH", 6, 7);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(TFT_CYAN, 0x2104);
    String ip = WiFi.localIP().toString();
    tft.drawString(ip, 314, 7);

    // Box 1: DASHBOARD
    tft.drawRoundRect(MENU_X1, MENU_Y1, MENU_BW, MENU_BH, 6, TFT_CYAN);
    tft.fillTriangle(MENU_X1 + 55, MENU_Y1 + 20, MENU_X1 + 45, MENU_Y1 + 55,
                     MENU_X1 + 65, MENU_Y1 + 55, TFT_GREEN);
    tft.fillRect(MENU_X1 + 70, MENU_Y1 + 25, 8, 35, TFT_RED);
    tft.fillRect(MENU_X1 + 82, MENU_Y1 + 35, 8, 25, TFT_YELLOW);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("DASHBOARD", MENU_X1 + MENU_BW / 2, MENU_Y1 + MENU_BH - 10);

    // Box 2: TELEMETRY
    tft.drawRoundRect(MENU_X2, MENU_Y1, MENU_BW, MENU_BH, 6, TFT_DARKGREY);
    tft.drawFastHLine(MENU_X2 + 40, MENU_Y1 + 55, 65, TFT_DARKGREY);
    tft.drawFastVLine(MENU_X2 + 40, MENU_Y1 + 20, 35, TFT_DARKGREY);
    tft.drawLine(MENU_X2 + 45, MENU_Y1 + 50, MENU_X2 + 60, MENU_Y1 + 30, TFT_CYAN);
    tft.drawLine(MENU_X2 + 60, MENU_Y1 + 30, MENU_X2 + 75, MENU_Y1 + 40, TFT_CYAN);
    tft.drawLine(MENU_X2 + 75, MENU_Y1 + 40, MENU_X2 + 95, MENU_Y1 + 25, TFT_CYAN);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("TELEMETRY", MENU_X2 + MENU_BW / 2, MENU_Y1 + MENU_BH - 10);

    // Box 3: LAPS
    tft.drawRoundRect(MENU_X1, MENU_Y2, MENU_BW, MENU_BH, 6, TFT_DARKGREY);
    for (int i = 0; i < 3; i++) {
        int ly = MENU_Y2 + 20 + i * 16;
        tft.fillCircle(MENU_X1 + 40, ly + 3, 2, TFT_GREEN);
        tft.drawFastHLine(MENU_X1 + 48, ly + 2, 50, TFT_DARKGREY);
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("LAPS", MENU_X1 + MENU_BW / 2, MENU_Y2 + MENU_BH - 10);

    // Box 4: SETTINGS
    tft.drawRoundRect(MENU_X2, MENU_Y2, MENU_BW, MENU_BH, 6, TFT_DARKGREY);
    int gcx = MENU_X2 + MENU_BW / 2, gcy = MENU_Y2 + 40;
    tft.drawCircle(gcx, gcy, 12, TFT_DARKGREY);
    tft.drawCircle(gcx, gcy, 6, TFT_WHITE);
    for (int a = 0; a < 360; a += 45) {
        float rad = a * DEG_TO_RAD;
        int tx = gcx + (int)(15 * cos(rad));
        int ty = gcy + (int)(15 * sin(rad));
        tft.fillRect(tx - 2, ty - 2, 4, 4, TFT_WHITE);
    }
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("SETTINGS", MENU_X2 + MENU_BW / 2, MENU_Y2 + MENU_BH - 10);
}

int handleMenuTouch(int tx, int ty) {
    // Returns screen index (0-3) or -1 if no box hit
    if (ty >= MENU_Y1 && ty < MENU_Y1 + MENU_BH) {
        if (tx >= MENU_X1 && tx < MENU_X1 + MENU_BW) return 0;  // Dashboard
        if (tx >= MENU_X2 && tx < MENU_X2 + MENU_BW) return 1;  // Telemetry
    }
    if (ty >= MENU_Y2 && ty < MENU_Y2 + MENU_BH) {
        if (tx >= MENU_X1 && tx < MENU_X1 + MENU_BW) return 2;  // Laps
        if (tx >= MENU_X2 && tx < MENU_X2 + MENU_BW) return 3;  // Settings
    }
    return -1;
}

// ── Setup ───────────────────────────────────────────────────────────────────

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable brownout detector
    setCpuFrequencyMhz(80);                       // 80MHz = less current, WiFi still works

    Serial.begin(115200);
    settingsLoad();
    displayInit();
    touchInit();

    drawConnectingScreen();

    WiFi.mode(WIFI_STA);
    WiFi.begin(g_settings.wifiSSID, g_settings.wifiPass);

    uint32_t wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiStart > 15000) {
            drawWifiFailed();
            Serial.println("WiFi connection failed!");
            while (true) { delay(1000); }
        }
        delay(250);
    }

    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    // Lower WiFi TX power to reduce current spikes
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(true);  // Enable modem sleep between packets

    telemetryInit(g_settings.udpPort);

    appState = STATE_WAITING;
    drawWaitingScreen();
}

// ── Loop ────────────────────────────────────────────────────────────────────

void loop() {
    uint32_t now = millis();
    if (now - lastDrawTime < FRAME_INTERVAL_MS) return;
    lastDrawTime = now;

    TelemetryData data = telemetrySnapshot();

    // ── Waiting state (Pit Lane screen) ─────────────────────────────────
    if (appState == STATE_WAITING) {
        updateWaitingDots();
        if (data.isRaceOn == 1) {
            appState = STATE_ACTIVE;
            lastRaceOnTime = now;
            currentScreenIdx = 0;
            screens[0]->enter();
            drawPageDots(0, NUM_SCREENS);
        }
        return;
    }

    // ── Active state ────────────────────────────────────────────────────
    if (data.isRaceOn == 1) {
        lastRaceOnTime = now;
    } else if (now - lastRaceOnTime > RACE_OFF_TIMEOUT_MS) {
        screens[currentScreenIdx]->leave();
        appState = STATE_WAITING;
        setLED(false, false, false);
        drawWaitingScreen();
        return;
    }

    // ── Touch navigation ────────────────────────────────────────────────
    int tx, ty;
    if (touchGetXY(tx, ty)) {
        if (!screens[currentScreenIdx]->handleTouch(tx, ty)) {
            if (tx < 160) {
                switchScreen((currentScreenIdx - 1 + NUM_SCREENS) % NUM_SCREENS);
            } else {
                switchScreen((currentScreenIdx + 1) % NUM_SCREENS);
            }
        }
    }

    // ── Update current screen ───────────────────────────────────────────
    screens[currentScreenIdx]->update(data);
}
