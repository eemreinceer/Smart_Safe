#include <Arduino.h>
#include <Preferences.h>
#include "eventlogger.h"

#define MAX_LOG_ENTRIES 50  // dairesel tampon — NVS taşmasını önler

static Preferences prefs;
static int logIndex = -1;  // RAM'de tutulur, her log'da NVS okuma yapılmaz

void initLogger()
{
    prefs.begin("logs", false);
    logIndex = prefs.getInt("index", 0);
    Serial.printf("[LOG] Logger hazir (index: %d)\n", logIndex);
}

void logEvent(String event)
{
    if (logIndex < 0) return;

    // Dairesel tampon: aynı 50 slot sürekli kullanılır
    String key = "e" + String(logIndex % MAX_LOG_ENTRIES);
    prefs.putString(key.c_str(), event);
    logIndex++;
    prefs.putInt("index", logIndex);
}
