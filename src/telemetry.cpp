#include "telemetry.h"
#include <AsyncUDP.h>

volatile TelemetryData g_telemetry = {};
static AsyncUDP udp;

static void onPacket(AsyncUDPPacket& packet) {
    if (packet.length() < FH4_PACKET_SIZE) return;
    const uint8_t* buf = packet.data();

    noInterrupts();
    g_telemetry.isRaceOn      = readS32(buf, 0);
    g_telemetry.engineMaxRpm  = readF32(buf, 8);
    g_telemetry.engineIdleRpm = readF32(buf, 12);
    g_telemetry.currentRpm    = readF32(buf, 16);
    g_telemetry.accelX        = readF32(buf, 20);
    g_telemetry.accelY        = readF32(buf, 24);
    g_telemetry.accelZ        = readF32(buf, 28);
    g_telemetry.speed         = readF32(buf, 256);
    g_telemetry.power         = readF32(buf, 260);
    g_telemetry.torque        = readF32(buf, 264);
    g_telemetry.boost         = readF32(buf, 284);
    g_telemetry.bestLap       = readF32(buf, 296);
    g_telemetry.lastLap       = readF32(buf, 300);
    g_telemetry.currentLap    = readF32(buf, 304);
    g_telemetry.currentRaceTime = readF32(buf, 308);
    g_telemetry.racePosition  = buf[314];
    g_telemetry.accel         = buf[315];
    g_telemetry.brake         = buf[316];
    g_telemetry.gear          = buf[319];
    interrupts();
}

void telemetryInit(uint16_t port) {
    if (udp.listen(port)) {
        udp.onPacket(onPacket);
        Serial.printf("Listening on UDP port %d\n", port);
    }
}

TelemetryData telemetrySnapshot() {
    TelemetryData local;
    noInterrupts();
    local = *(const TelemetryData*)&g_telemetry;
    interrupts();
    return local;
}
