#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "lock.h"

void initLock()
{
    Serial.println("[MOCK-LOCK] Kilit simülasyonu hazır");
}

void unlockSafe()
{
    Serial.println("══════════════════════════════════");
    Serial.println("  🔓 KASA KİLİDİ AÇILDI");
    Serial.printf( "  Süre: %d ms sonra kapanacak\n", LOCK_OPEN_DURATION_MS);
    Serial.println("══════════════════════════════════");

    // Non-blocking bekleme
    vTaskDelay(pdMS_TO_TICKS(LOCK_OPEN_DURATION_MS));

    Serial.println("[MOCK-LOCK] 🔒 Kilit tekrar kapandı");
}

void lockSafe()
{
    Serial.println("[MOCK-LOCK] 🔒 Kilit kilitlendi");
}

#endif