#include "security_manager.h"
#include "config.h"
#include "systemstate.h"
#include "camera.h"
#include "lock.h"
#include "alarm.h"
#include "firebase.h"
#include "offlinequeue.h"
#include "eventlogger.h"

#include <Arduino.h>
#include <time.h>

// ─── Event Queue ─────────────────────────────
QueueHandle_t eventQueue = NULL;

// ─── Dahili state — dışarıya accessor ile açık
static volatile int           s_failCount = 0;
static volatile unsigned long s_lockUntil = 0;

// ─────────────────────────────────────────────
void securityInit()
{
    eventQueue = xQueueCreate(10, sizeof(SafeEvent));
    configASSERT(eventQueue != NULL);
}

// ─────────────────────────────────────────────
bool securityIsLockedDown()
{
    return s_lockUntil > 0 && millis() < s_lockUntil;
}

unsigned long securityGetLockUntil()
{
    return s_lockUntil;
}

// ─── Lockdown süre kontrolü (taskSystem'den) ─
void securityCheckExpiry()
{
    if (s_lockUntil > 0 &&
        millis() >= s_lockUntil &&
        currentState == STATE_LOCKDOWN)
    {
        s_lockUntil = 0;
        setState(STATE_IDLE);
        Serial.println("[SEC] Lockdown suresi doldu -> IDLE");
    }
}

// ─── Ana olay işleyici ───────────────────────
void securityHandleEvent(const SafeEvent& ev)
{
    if (securityIsLockedDown())
    {
        setState(STATE_LOCKDOWN);
        Serial.println("[SEC] Lockdown aktif, event reddedildi");
        return;
    }

    // ── YETKİLİ ──────────────────────────────
    if (ev.type == EVENT_AUTHORIZED)
    {
        Serial.printf("[SEC] Yetkili giris: %s\n", ev.id);
        setState(STATE_AUTHORIZED);

        const uint32_t eventTimestamp = static_cast<uint32_t>(time(nullptr));
        unlockSafe();

        if (!sendLog("AUTHORIZED", String(ev.id), "", eventTimestamp))
            storeOfflineLog("AUTHORIZED", String(ev.id), eventTimestamp);

        logEvent(String("AUTHORIZED ") + ev.id);
        s_failCount = 0;
        setState(STATE_IDLE);
    }

    // ── YETKİSİZ ─────────────────────────────
    else if (ev.type == EVENT_UNAUTHORIZED)
    {
        Serial.println("[SEC] YETKISIZ ERISIM!");
        setState(STATE_UNAUTHORIZED);

        String photo = "";
        const uint32_t eventTimestamp = static_cast<uint32_t>(time(nullptr));
        if (ESP.getFreeHeap() > 90000)
            photo = capturePhotoBase64();

        if (!sendLog("UNAUTHORIZED", String(ev.id), photo, eventTimestamp))
            storeOfflineLog("UNAUTHORIZED", String(ev.id), eventTimestamp);

        logEvent("UNAUTHORIZED ACCESS");

        s_failCount++;
        Serial.printf("[SEC] Basarisiz: %d/%d\n", s_failCount, MAX_FAIL_ATTEMPTS);

        if (s_failCount >= MAX_FAIL_ATTEMPTS)
        {
            s_lockUntil = millis() + LOCKDOWN_DURATION_MS;
            s_failCount = 0;
            setState(STATE_LOCKDOWN);
            triggerAlarm();
            logEvent("LOCKDOWN ACTIVATED");
            Serial.println("[SEC] LOCKDOWN AKTIF!");
        }
        else
        {
            setState(STATE_IDLE);
        }
    }
}
