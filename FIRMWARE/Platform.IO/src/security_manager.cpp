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
static unsigned long s_lockUntil = 0;
static unsigned long s_alarmUntil = 0;
static portMUX_TYPE s_securityMux = portMUX_INITIALIZER_UNLOCKED;

static bool deadlinePending(unsigned long deadline)
{
    return deadline != 0 && static_cast<long>(millis() - deadline) < 0;
}

// ─────────────────────────────────────────────
void securityInit()
{
    eventQueue = xQueueCreate(10, sizeof(SafeEvent));
    configASSERT(eventQueue != NULL);
}

// ─────────────────────────────────────────────
bool securityIsLockedDown()
{
    portENTER_CRITICAL(&s_securityMux);
    const unsigned long deadline = s_lockUntil;
    portEXIT_CRITICAL(&s_securityMux);
    return deadlinePending(deadline);
}

unsigned long securityGetLockUntil()
{
    portENTER_CRITICAL(&s_securityMux);
    const unsigned long deadline = s_lockUntil;
    portEXIT_CRITICAL(&s_securityMux);
    return deadline;
}

unsigned long securityGetLockRemainingMs()
{
    const unsigned long deadline = securityGetLockUntil();
    return deadlinePending(deadline) ? deadline - millis() : 0;
}

// ─── Lockdown süre kontrolü (taskSystem'den) ─
void securityCheckExpiry()
{
    portENTER_CRITICAL(&s_securityMux);
    const bool lockExpired = s_lockUntil > 0 && !deadlinePending(s_lockUntil);
    if (lockExpired) s_lockUntil = 0;
    const bool alarmExpired = s_alarmUntil > 0 && !deadlinePending(s_alarmUntil);
    if (alarmExpired) s_alarmUntil = 0;
    portEXIT_CRITICAL(&s_securityMux);

    if (lockExpired)
    {
        if (getState() == STATE_LOCKDOWN)
            setState(STATE_IDLE);
        stopAlarm();
        Serial.println("[SEC] Lockdown suresi doldu -> IDLE");
    }

    if (alarmExpired)
    {
        stopAlarm();
        if (getState() == STATE_ALARM)
            setState(securityIsLockedDown() ? STATE_LOCKDOWN : STATE_IDLE);
        Serial.println("[SEC] Alarm suresi doldu");
    }
}

// ─── Ana olay işleyici ───────────────────────
void securityHandleEvent(const SafeEvent& ev)
{
    if (ev.type == EVENT_REMOTE_ALARM)
    {
        lockSafe();
        setState(STATE_ALARM);
        triggerAlarm();
        portENTER_CRITICAL(&s_securityMux);
        s_alarmUntil = millis() + ALARM_DURATION_MS;
        portEXIT_CRITICAL(&s_securityMux);
        logEvent("REMOTE ALARM");
        return;
    }

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
            portENTER_CRITICAL(&s_securityMux);
            s_lockUntil = millis() + LOCKDOWN_DURATION_MS;
            portEXIT_CRITICAL(&s_securityMux);
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
