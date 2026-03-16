#include "settings.h"
#include "wifi_credentials.h"
#include <Preferences.h>

AppSettings g_settings;
static Preferences prefs;

void settingsLoad() {
    prefs.begin("forza", true);  // read-only

    strlcpy(g_settings.wifiSSID, prefs.getString("ssid", DEFAULT_WIFI_SSID).c_str(), sizeof(g_settings.wifiSSID));
    strlcpy(g_settings.wifiPass, prefs.getString("pass", DEFAULT_WIFI_PASS).c_str(), sizeof(g_settings.wifiPass));
    g_settings.udpPort        = prefs.getUShort("port", 5300);
    g_settings.useMetric      = prefs.getBool("metric", true);
    g_settings.brightness     = prefs.getUChar("bright", 128);
    g_settings.shiftFlashPct  = prefs.getFloat("sFlash", 0.90f);
    g_settings.shiftGreenPct  = prefs.getFloat("sGreen", 0.75f);
    g_settings.shiftYellowPct = prefs.getFloat("sYellow", 0.85f);
    g_settings.shiftRedPct    = prefs.getFloat("sRed", 0.92f);

    prefs.end();
}

void settingsSave() {
    prefs.begin("forza", false);  // read-write

    prefs.putString("ssid", g_settings.wifiSSID);
    prefs.putString("pass", g_settings.wifiPass);
    prefs.putUShort("port", g_settings.udpPort);
    prefs.putBool("metric", g_settings.useMetric);
    prefs.putUChar("bright", g_settings.brightness);
    prefs.putFloat("sFlash", g_settings.shiftFlashPct);
    prefs.putFloat("sGreen", g_settings.shiftGreenPct);
    prefs.putFloat("sYellow", g_settings.shiftYellowPct);
    prefs.putFloat("sRed", g_settings.shiftRedPct);

    prefs.end();
}
