#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "alarm.h"

void initAlarm()
{
    Serial.println("[MOCK-ALARM] Alarm simülasyonu hazır");
}

void triggerAlarm()
{
    Serial.println("╔══════════════════════════════════╗");
    Serial.println("║  🚨🚨🚨 ALARM ÇALIYOR!!! 🚨🚨🚨  ║");
    Serial.println("╚══════════════════════════════════╝");
}

void stopAlarm()
{
    Serial.println("[MOCK-ALARM] Alarm durduruldu");
}

#endif
