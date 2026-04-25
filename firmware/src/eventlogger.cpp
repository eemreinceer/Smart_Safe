#include <Arduino.h>
#include <Preferences.h>

#include "eventlogger.h"

Preferences prefs;

void initLogger()
{
    prefs.begin("logs", false);
}

void logEvent(String event)
{
    int index = prefs.getInt("index", 0);

    String key = "log" + String(index);

    prefs.putString(key.c_str(), event);

    prefs.putInt("index", index + 1);

    Serial.println("Log stored: " + event);
}