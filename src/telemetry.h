#pragma once
#include <Arduino.h>

#define FH4_PACKET_SIZE 324

struct TelemetryData {
    // Sled section
    int32_t  isRaceOn;        // offset 0
    float    engineMaxRpm;    // offset 8
    float    engineIdleRpm;   // offset 12
    float    currentRpm;      // offset 16
    float    accelX;          // offset 20 (lateral G, m/s^2)
    float    accelY;          // offset 24 (vertical, m/s^2)
    float    accelZ;          // offset 28 (longitudinal, m/s^2)

    // Dash section (FH4 offsets, include +12 shift)
    float    speed;           // offset 256 (m/s)
    float    power;           // offset 260 (watts)
    float    torque;          // offset 264 (Nm)
    float    tireTempFL;     // offset 268 (fahrenheit)
    float    tireTempFR;     // offset 272
    float    tireTempRL;     // offset 276
    float    tireTempRR;     // offset 280
    float    boost;           // offset 284 (atmospheres)
    float    bestLap;         // offset 296 (seconds)
    float    lastLap;         // offset 300 (seconds)
    float    currentLap;      // offset 304 (seconds)
    float    currentRaceTime; // offset 308 (seconds)
    uint16_t lapNumber;      // offset 312
    uint8_t  racePosition;   // offset 314
    uint8_t  accel;           // offset 315
    uint8_t  brake;           // offset 316
    uint8_t  gear;            // offset 319 (0=R, 1-10=forward)
};

inline float readF32(const uint8_t* buf, int offset) {
    float v;
    memcpy(&v, buf + offset, 4);
    return v;
}

inline int32_t readS32(const uint8_t* buf, int offset) {
    int32_t v;
    memcpy(&v, buf + offset, 4);
    return v;
}

extern volatile TelemetryData g_telemetry;

void telemetryInit(uint16_t port);
TelemetryData telemetrySnapshot();
