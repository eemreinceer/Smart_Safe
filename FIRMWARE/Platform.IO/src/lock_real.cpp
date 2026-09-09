#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "lock.h"

static volatile bool s_isLocked = true;
static volatile unsigned long s_unlockUntil = 0;

void initLock()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    s_isLocked = true;
    s_unlockUntil = 0;
    Serial.println("[LOCK] ✅ Kilit hazır");
}

void unlockSafe()
{
    Serial.println("[LOCK] 🔓 Kilit açılıyor...");

    digitalWrite(RELAY_PIN, HIGH);
    s_isLocked = false;
    s_unlockUntil = millis() + LOCK_OPEN_DURATION_MS;
}

void lockSafe()
{
    Serial.println("[LOCK] 🔒 Kilit kilitlendi");
    digitalWrite(RELAY_PIN, LOW);
    s_isLocked = true;
    s_unlockUntil = 0;
}

void serviceLock()
{
    if (!s_isLocked && static_cast<long>(millis() - s_unlockUntil) >= 0)
    {
        digitalWrite(RELAY_PIN, LOW);
        s_isLocked = true;
        s_unlockUntil = 0;
        Serial.println("[LOCK] 🔒 Kilit kapandı");
    }
}

bool isSafeLocked()
{
    return s_isLocked;
}

#endif
