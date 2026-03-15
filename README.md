# Forza Horizon 4 Telemetry Dashboard — ESP32-2432S028 (CYD)

Real-time racing dashboard on the "Cheap Yellow Display" that receives Forza Horizon 4 UDP telemetry over WiFi.

## Features

- RPM bar with color zones (green → yellow → red)
- 9-segment shift light indicator with flash at redline
- Large speed (MPH) and gear readout
- Boost (PSI), horsepower, and torque display
- Accel/brake pedal bars
- Physical RGB LED shift light mirroring on-screen state

## Setup

### 1. WiFi Credentials

Open `src/main.cpp` and edit these two lines near the top:

```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
```

### 2. Flash

```
pio run -t upload
```

### 3. Configure Forza Horizon 4

1. Launch FH4, go to **Settings → HUD & Gameplay**
2. Scroll to **Data Out**
3. Set **Data Out** to **ON**
4. Set **Data Out IP Address** to the IP shown on the CYD screen
5. Set **Data Out Port** to `5300`

The dashboard will automatically switch from the waiting screen to the live view once telemetry packets arrive.

## Troubleshooting

### Screen stays black or shows garbage

The CYD ships with either an ILI9341 or ST7789 driver IC. If the display doesn't work, edit `platformio.ini` and change:

```
-DILI9341_2_DRIVER
```

to:

```
-DST7789_2_DRIVER
```

Then re-flash.

### No data appearing

- Verify ESP32 and PC/Xbox are on the same network
- Check that the IP and port match what's shown on the waiting screen
- FH4 only sends data while actively driving (race, freeroam, etc.) — it stops in menus and pause screens

## Shift Light Thresholds

The shift lights activate based on the percentage of max RPM:

| Threshold | % of Max RPM | On-Screen | RGB LED |
|-----------|-------------|-----------|---------|
| Green     | 75%         | Segments 1-4 light green | Green |
| Yellow    | 85%         | Segments 5-7 light yellow | Yellow (R+G) |
| Red       | 92%         | Segments 8-9 light red | Solid red |
| Flash     | 97%         | All segments flash | Flashing red |

To tune these, edit the `THRESH_*` defines in `main.cpp`:

```cpp
#define THRESH_GREEN  0.75f
#define THRESH_YELLOW 0.85f
#define THRESH_RED    0.92f
#define THRESH_FLASH  0.97f
```
