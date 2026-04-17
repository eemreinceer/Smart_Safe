#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "alarm.h"
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfplayer;

void initAlarm()
{
    dfSerial.begin(9600, SERIAL_8N1, DF_RX, DF_TX);

    if (!dfplayer.begin(dfSerial))
        Serial.println("[ALARM] ❌ DFPlayer hatası");
    else
        Serial.println("[ALARM] ✅ DFPlayer hazır");
}

void triggerAlarm()
{
    Serial.println("[ALARM] 🚨 Alarm tetiklendi!");
    dfplayer.play(1);
}

#endif