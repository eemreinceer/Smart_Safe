#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "lock.h"

static volatile bool s_isLocked = true;
static volatile unsigned long s_unlockUntil = 0;

void initLock()
{
    s_isLocked = true;
    s_unlockUntil = 0;
    Serial.println("[MOCK-LOCK] Kilit simülasyonu hazır");
}

void unlockSafe()
{
    Serial.println("══════════════════════════════════");
    Serial.println("  🔓 KASA KİLİDİ AÇILDI");
    Serial.printf( "  Süre: %d ms sonra kapanacak\n", LOCK_OPEN_DURATION_MS);
    Serial.println("══════════════════════════════════");

    s_isLocked = false;
    s_unlockUntil = millis() + LOCK_OPEN_DURATION_MS;
}

void lockSafe()
{
    s_isLocked = true;
    s_unlockUntil = 0;
    Serial.println("[MOCK-LOCK] 🔒 Kilit kilitlendi");
}

void serviceLock()
{
    if (!s_isLocked && static_cast<long>(millis() - s_unlockUntil) >= 0)
    {
        s_isLocked = true;
        s_unlockUntil = 0;
        Serial.println("[MOCK-LOCK] 🔒 Kilit tekrar kapandı");
    }
}

bool isSafeLocked()
{
    return s_isLocked;
}

#endif
