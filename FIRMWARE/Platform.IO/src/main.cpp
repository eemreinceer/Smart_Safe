#include <Arduino.h>

#include "config.h"
#include "events.h"
#include "wifi_manager.h"
#include "security_manager.h"
#include "rfid.h"
#include "camera.h"
#include "lock.h"
#include "alarm.h"
#include "firebase.h"
#include "ota.h"
#include "eventlogger.h"
#include "offlinequeue.h"
#include "systemstate.h"

#include "esp_task_wdt.h"

// ─────────────────────────────────────────────
//  Forward Declarations
// ─────────────────────────────────────────────
void taskRFID    (void* pv);
void taskSecurity(void* pv);
void taskCloud   (void* pv);
void taskWifi    (void* pv);
void taskSystem  (void* pv);


// ══════════════════════════════════════════════
//  TASK 1 — RFID Okuyucu  |  Core 1  |  Prio 2
// ══════════════════════════════════════════════
void taskRFID(void* pv)
{
    esp_task_wdt_add(NULL);
    while (true)
    {
        if (securityIsLockedDown() ||
            getState() == STATE_LOCKDOWN ||
            getState() == STATE_ALARM)
        {
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int cardID = readRFID();

        SafeEvent ev;
        memset(&ev, 0, sizeof(ev));

        if (cardID > 0)
        {
            ev.type = EVENT_AUTHORIZED;
            snprintf(ev.id, sizeof(ev.id), "KART-%d", cardID);
            setState(STATE_AUTHORIZED);
            xQueueSend(eventQueue, &ev, portMAX_DELAY);
        }
        else if (cardID == -2)
        {
            ev.type = EVENT_UNAUTHORIZED;
            snprintf(ev.id, sizeof(ev.id), "UNKNOWN");
            setState(STATE_UNAUTHORIZED);
            xQueueSend(eventQueue, &ev, portMAX_DELAY);
        }
        else
        {
            if (getState() != STATE_LOCKDOWN &&
                getState() != STATE_ALARM)
                setState(STATE_IDLE);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
        esp_task_wdt_reset();
    }
}


// ══════════════════════════════════════════════
//  TASK 2 — Security  |  Core 1  |  Prio 2
// ══════════════════════════════════════════════
void taskSecurity(void* pv)
{
    esp_task_wdt_add(NULL);
    SafeEvent ev;

    while (true)
    {
        if (xQueueReceive(eventQueue, &ev, pdMS_TO_TICKS(8000)) != pdTRUE)
        {
            esp_task_wdt_reset();
            continue;
        }

        securityHandleEvent(ev);
        esp_task_wdt_reset();
    }
}


// ══════════════════════════════════════════════
//  TASK 3 — Cloud  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskCloud(void* pv)
{
    esp_task_wdt_add(NULL);
    unsigned int counter = 0;
    bool lastPublishedLockState = isSafeLocked();

    while (true)
    {
        handleOTA();

        const bool lockState = isSafeLocked();
        if (lockState != lastPublishedLockState && wifiIsConnected())
        {
            if (updateLockState(lockState))
                lastPublishedLockState = lockState;
        }

        if (counter % 5 == 0 && wifiIsConnected())
        {
            if (!firebaseIsReady() && firebaseIsConfigured())
                initFirebase();

            trySyncOfflineLogs();

            bool alarmTrigger = false;
            if (checkRemoteCommands(alarmTrigger))
            {
                if (alarmTrigger)
                {
                    Serial.println("[CLOUD] Uzaktan alarm!");
                    SafeEvent rem;
                    memset(&rem, 0, sizeof(rem));
                    rem.type = EVENT_REMOTE_ALARM;
                    if (xQueueSend(eventQueue, &rem, 0) != pdTRUE)
                        Serial.println("[CLOUD] Security queue dolu; alarm reddedildi");
                }
            }
        }

        if (counter % 60 == 0 && wifiIsConnected())
            updateDeviceStatus();

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_task_wdt_reset();
    }
}


// ══════════════════════════════════════════════
//  TASK 4 — WiFi Monitor  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskWifi(void* pv)
{
    esp_task_wdt_add(NULL);
    while (true)
    {
        wifiReconnect();
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


// ══════════════════════════════════════════════
//  TASK 5 — System Monitor  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskSystem(void* pv)
{
    esp_task_wdt_add(NULL);
    unsigned long lastReportAt = 0;
    while (true)
    {
        securityCheckExpiry();
        serviceLock();

        if (millis() - lastReportAt >= 5000)
        {
            lastReportAt = millis();
            Serial.printf("[SYS] Heap: %u | State: %s | WiFi: %s\n",
                          ESP.getFreeHeap(),
                          getStateName(getState()),
                          wifiIsConnected() ? "OK" : "DOWN");

            const unsigned long remaining = securityGetLockRemainingMs();
            if (remaining > 0)
                Serial.printf("[SYS] Lockdown: %lu ms kaldi\n", remaining);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
        esp_task_wdt_reset();
    }
}


// ══════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n========================================");
    Serial.println("[SETUP] Akilli Guvenlik Kasasi basliyor");
    Serial.println("========================================");

    #if SIMULATION_MODE
    Serial.println("[SETUP] SIMULASYON MODU");
    #endif
    #if RFID_MOCK
    Serial.println("[SETUP] RFID MOCK - Komut: RFID:AABBCCDD");
    #endif

    // ── Donanım ──────────────────────────────
    initLogger();
    initOfflineQueue();
    initRFID();
    initCamera();
    initLock();
    initAlarm();

    // ── Ağ (WiFi + NTP) ──────────────────────
    wifiInit();

    // ── Bulut ────────────────────────────────
    initFirebase();
    initOTA();

    // ── Güvenlik state makinesi ───────────────
    securityInit();
    setState(STATE_IDLE);

    // ── WDT: 30 sn (SSL 8 sn + işlem payı) ──
    esp_task_wdt_init(30, true);
    esp_task_wdt_add(NULL);

    if (eventQueue == NULL)
    {
        Serial.println("[SETUP] eventQueue NULL! Sistem durdu.");
        while (true);
    }

    // ── Task'lar ─────────────────────────────
    bool tasksOk = true;
    tasksOk = xTaskCreatePinnedToCore(taskRFID,     "rfid",     4096,  NULL, 2, NULL, 1) == pdPASS && tasksOk;
    tasksOk = xTaskCreatePinnedToCore(taskSecurity, "security", 12288, NULL, 2, NULL, 1) == pdPASS && tasksOk;
    tasksOk = xTaskCreatePinnedToCore(taskCloud,    "cloud",    12288, NULL, 1, NULL, 0) == pdPASS && tasksOk;
    tasksOk = xTaskCreatePinnedToCore(taskWifi,     "wifi",     4096,  NULL, 1, NULL, 0) == pdPASS && tasksOk;
    tasksOk = xTaskCreatePinnedToCore(taskSystem,   "system",   2048,  NULL, 1, NULL, 0) == pdPASS && tasksOk;

    if (!tasksOk)
    {
        lockSafe();
        Serial.println("[SETUP] Task olusturma basarisiz; kilit safe state'te.");
        esp_restart();
    }

    Serial.println("[SETUP] SMART SAFE READY\n");
}


// ══════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════
void loop()
{
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
