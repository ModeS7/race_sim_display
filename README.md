# Forza Horizon 4 Telemetry Dashboard — ESP32-2432S028 (CYD)

Real-time racing dashboard on the ESP32-2432S028 "Cheap Yellow Display" that receives Forza Horizon 4 UDP telemetry over WiFi and renders live data on the built-in 320x240 TFT.

## Features

- **Multi-screen dashboard** — swipe left/right between screens:
  - **Race Dash** — large 7-segment gear indicator, speed, RPM bar with color zones, shift lights, position, pedal bars, lap times
  - **Data Dash** — same core layout plus large current lap timer, best/last lap comparison
  - **Lap Info** — detailed lap times, race position, speed and gear
  - **Settings** — toggle metric/imperial, adjust brightness, tune shift flash threshold, save to flash
- **Full-screen shift flash** — screen flashes red with "SHIFT!" at configurable RPM threshold, alternating with the live dashboard so you never lose data
- **7-segment gear display** — large custom-drawn gear indicator in the center of the screen
- **9-segment shift light bar** — green/yellow/red zones with configurable thresholds
- **Physical RGB LED shift light** — onboard LED mirrors the shift state (green → yellow → red → flashing)
- **Touch navigation** — tap left/right half of screen to switch between screens
- **Persistent settings** — saved to ESP32 NVS flash, survives reboots
- **Styled UI** — Race Stripe boot screen, Pit Lane animated waiting screen with scanning dots

## Hardware

- **Board**: ESP32-2432S028 (aka "Cheap Yellow Display" / CYD)
- **Display**: 2.8" 320x240 ILI9341 TFT (SPI/HSPI)
- **Touch**: XPT2046 resistive touchscreen (separate SPI/VSPI)
- **Onboard RGB LED**: pins 4 (R), 16 (G), 17 (B) — active LOW

## Setup

### 1. Install PlatformIO

If you don't have PlatformIO, install it via pip:

```bash
python -m venv .venv

# Windows
.venv\Scripts\activate

# Linux/Mac
source .venv/bin/activate

pip install platformio
```

### 2. WiFi Credentials

Copy the template and fill in your WiFi details:

```bash
cp src/wifi_credentials.h.template src/wifi_credentials.h
```

Edit `src/wifi_credentials.h`:

```cpp
#define DEFAULT_WIFI_SSID "YOUR_SSID"
#define DEFAULT_WIFI_PASS "YOUR_PASSWORD"
```

### 3. Build and Flash

Connect the CYD via USB and run:

```bash
pio run -t upload
```

If the upload fails mid-way, try a different USB cable or port. Hold the BOOT button on the CYD during upload if it won't connect.

### 4. Configure Forza Horizon 4

1. The CYD will show its IP address on the waiting screen after connecting to WiFi
2. Launch FH4, go to **Settings > HUD & Gameplay**
3. Scroll to **Data Out**
4. Set **Data Out** to **ON**
5. Set **Data Out IP Address** to the IP shown on the CYD
6. Set **Data Out Port** to `5300`
7. Start driving — the dashboard appears automatically

## Project Structure

```
src/
  main.cpp              - Setup, WiFi, screen manager, boot/waiting screens
  telemetry.h/cpp       - UDP listener, FH4 packet parsing, TelemetryData struct
  display.h/cpp         - TFT init, LED control, page dots, ScreenBase interface
  touch.h/cpp           - XPT2046 touch on VSPI, debounced edge detection
  settings.h/cpp        - Persistent settings via ESP32 Preferences (NVS)
  seg7.h/cpp            - Custom 7-segment digit renderer
  screen_dash.h/cpp     - Race Dash screen (gear, speed, RPM, position, pedals, laps)
  screen_dash_ext.h/cpp - Data Dash screen (adds large lap timer, best/last comparison)
  screen_laps.h/cpp     - Lap times detail screen
  screen_settings.h/cpp - Settings screen with touch controls
  wifi_credentials.h            - Your WiFi credentials (gitignored)
  wifi_credentials.h.template   - Template for credentials
```

## Troubleshooting

### Screen stays white or shows wrong colors

The CYD ships with different display driver ICs depending on the batch:

| Variant | USB Ports | Driver Flag |
|---------|-----------|-------------|
| v1/v2 (most common) | 1x micro-USB | `-DILI9341_2_DRIVER` (default) |
| v3 "2USB" | micro-USB + USB-C | `-DST7789_DRIVER` |

If the display doesn't work, edit `platformio.ini` and change the driver flag, then re-flash.

### Screen shows colors but no text

Make sure the font flags are present in `platformio.ini` build_flags:

```
-DLOAD_GLCD
-DLOAD_FONT2
-DLOAD_FONT4
-DLOAD_FONT6
-DLOAD_FONT7
-DLOAD_FONT8
-DLOAD_GFXFF
```

### No telemetry data

- ESP32 and PC/Xbox must be on the **same WiFi network**
- IP and port must match what the CYD displays on the waiting screen
- FH4 only sends data while **actively driving** (race, freeroam, etc.) — it pauses in menus
- The dashboard stays visible for 20 seconds after pausing/rewinding before returning to the waiting screen

### Touch not responding

The CYD's touchscreen uses separate SPI pins from the display. If touch doesn't work:
- Verify the XPT2046_Touchscreen library is installed (should be automatic via `lib_deps`)
- The touch coordinate mapping may need calibration for your specific board — adjust the `map()` values in `src/touch.cpp`

## Shift Light Thresholds

Configurable via the Settings screen (swipe to screen 4) or by editing defaults in `src/settings.cpp`:

| Zone | Default % | On-Screen | RGB LED |
|------|----------|-----------|---------|
| Green | 75% | Segments 1-4 green | Green |
| Yellow | 85% | Segments 5-7 yellow | Yellow (R+G) |
| Red | 92% | Segments 8-9 red | Solid red |
| Flash | 90% | Full screen red + "SHIFT!" | Flashing red |

The flash threshold can be adjusted in Settings from 85% to 97%.

## Powering from a Racing Wheel

If mounting near a Logitech G920/G29, you can power the CYD from the wheel base's internal 5V rail instead of running a separate USB cable. The wheel rim connector Pin 7 provides 5V, Pin 1 is GND. See the [Arduino forum custom wheel threads](https://forum.arduino.cc/t/custom-steering-wheel-for-logitech-g29/1338749) for pinout details.

## FH4 Telemetry Packet Format

The dashboard parses the 324-byte "Car Dash" UDP packet. Key offsets (FH4-specific, include the +12 byte Horizon shift):

| Offset | Type | Field |
|--------|------|-------|
| 0 | s32 | IsRaceOn |
| 8 | f32 | EngineMaxRpm |
| 12 | f32 | EngineIdleRpm |
| 16 | f32 | CurrentEngineRpm |
| 20 | f32 | AccelerationX |
| 24 | f32 | AccelerationY |
| 28 | f32 | AccelerationZ |
| 256 | f32 | Speed (m/s) |
| 260 | f32 | Power (watts) |
| 264 | f32 | Torque (Nm) |
| 284 | f32 | Boost (atmospheres) |
| 296 | f32 | BestLap (seconds) |
| 300 | f32 | LastLap (seconds) |
| 304 | f32 | CurrentLap (seconds) |
| 314 | u8 | RacePosition |
| 315 | u8 | Accel (0-255) |
| 316 | u8 | Brake (0-255) |
| 319 | u8 | Gear (0=R, 1-10) |

## License

MIT
