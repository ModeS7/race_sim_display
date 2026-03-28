#include "shift_advisor.h"
#include "settings.h"

ShiftAdvisor g_shiftAdvisor;

void ShiftAdvisor::update(const TelemetryData& d) {
    advice = SHIFT_NONE;

    if (d.engineMaxRpm <= 0.0f || d.currentRpm <= 0.0f) return;

    float rpmPct = d.currentRpm / d.engineMaxRpm;

    // ── Detect car change (reset learning) ──────────────────────────────
    if (fabsf(d.engineMaxRpm - lastEngineMaxRpm) > 100.0f) {
        peakPowerRpm = 0.0f;
        peakPowerSeen = 0.0f;
        lastEngineMaxRpm = d.engineMaxRpm;
    }

    if (g_settings.shiftMode == SHIFT_MODE_PERCENT) {
        // ── Fixed percentage mode ───────────────────────────────────────
        if (rpmPct >= g_settings.shiftFlashPct) {
            advice = SHIFT_UPSHIFT;
        } else if (rpmPct < g_settings.shiftGreenPct * 0.6f && d.speed > 5.0f && d.gear > 1) {
            advice = SHIFT_DOWNSHIFT;
        }

    } else {
        // ── Adaptive mode ───────────────────────────────────────────────

        // Learn: track RPM where highest power occurs
        // Only learn when fully on throttle, in gear, and actually making power
        if (d.accel > 200 && d.gear >= 1 && d.power > 0.0f) {
            if (d.power > peakPowerSeen) {
                peakPowerSeen = d.power;
                peakPowerRpm = d.currentRpm;
            }
        }

        // ── Upshift logic ───────────────────────────────────────────────
        if (peakPowerRpm > 0.0f) {
            float upshiftRpm = peakPowerRpm * 1.03f;
            // Cap at 95% of max RPM only in 1st and 2nd gear
            if (d.gear <= 2) {
                float limiterRpm = d.engineMaxRpm * 0.95f;
                if (upshiftRpm > limiterRpm) upshiftRpm = limiterRpm;
            }

            if (d.currentRpm >= upshiftRpm) {
                advice = SHIFT_UPSHIFT;
            }
        } else {
            // Haven't learned yet — fall back to 92% as a safe default
            if (rpmPct >= 0.92f) {
                advice = SHIFT_UPSHIFT;
            }
        }

        // ── Downshift logic ─────────────────────────────────────────────
        if (d.speed > 5.0f && d.gear > 1) {
            float downshiftPct;
            if (peakPowerRpm > 0.0f) {
                // Below 60% of peak power RPM
                downshiftPct = (peakPowerRpm * 0.60f) / d.engineMaxRpm;
            } else {
                // Haven't learned — use 35% of max RPM
                downshiftPct = 0.35f;
            }
            if (rpmPct < downshiftPct) {
                advice = SHIFT_DOWNSHIFT;
            }
        }
    }
}
