#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "lock.h"

void initLock()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("[LOCK] ✅ Kilit hazır");
}

void unlockSafe()
{
    Serial.println("[LOCK] 🔓 Kilit açılıyor...");

    digitalWrite(RELAY_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(LOCK_OPEN_DURATION_MS));
    digitalWrite(RELAY_PIN, LOW);

    Serial.println("[LOCK] 🔒 Kilit kapandı");
}

void lockSafe()
{
    Serial.println("[LOCK] 🔒 Kilit kilitlendi");
    digitalWrite(RELAY_PIN, LOW);
}

#endif