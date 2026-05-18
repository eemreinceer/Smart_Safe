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
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
    while (true)
    {
        if (securityIsLockedDown() ||
            currentState == STATE_LOCKDOWN ||
            currentState == STATE_ALARM)
        {
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
            if (currentState != STATE_LOCKDOWN &&
                currentState != STATE_ALARM)
                setState(STATE_IDLE);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


// ══════════════════════════════════════════════
//  TASK 2 — Security  |  Core 1  |  Prio 2
// ══════════════════════════════════════════════
void taskSecurity(void* pv)
{
    SafeEvent ev;

    while (true)
    {
        if (xQueueReceive(eventQueue, &ev, pdMS_TO_TICKS(8000)) != pdTRUE)
            continue;

        securityHandleEvent(ev);
    }
}


// ══════════════════════════════════════════════
//  TASK 3 — Cloud  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskCloud(void* pv)
{
    unsigned int counter = 0;

    while (true)
    {
        handleOTA();

        if (counter % 5 == 0 && wifiIsConnected())
        {
            trySyncOfflineLogs();

            bool alarmTrigger = false, remoteUnlock = false;
            if (checkRemoteCommands(alarmTrigger, remoteUnlock))
            {
                if (alarmTrigger)
                {
                    Serial.println("[CLOUD] Uzaktan alarm!");
                    setState(STATE_ALARM);
                    triggerAlarm();
                }
                if (remoteUnlock)
                {
                    Serial.println("[CLOUD] Uzaktan kilit acma!");
                    SafeEvent rem;
                    memset(&rem, 0, sizeof(rem));
                    rem.type = EVENT_AUTHORIZED;
                    snprintf(rem.id, sizeof(rem.id), "REMOTE");
                    xQueueSend(eventQueue, &rem, 0);
                }
            }
        }

        if (counter % 60 == 0 && wifiIsConnected())
            updateDeviceStatus();

        counter++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// ══════════════════════════════════════════════
//  TASK 4 — WiFi Monitor  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskWifi(void* pv)
{
    while (true)
    {
        wifiReconnect();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


// ══════════════════════════════════════════════
//  TASK 5 — System Monitor  |  Core 0  |  Prio 1
// ══════════════════════════════════════════════
void taskSystem(void* pv)
{
    while (true)
    {
        securityCheckExpiry();

        Serial.printf("[SYS] Heap: %u | State: %s | WiFi: %s\n",
                      ESP.getFreeHeap(),
                      getStateName(currentState),
                      wifiIsConnected() ? "OK" : "DOWN");

        unsigned long lu = securityGetLockUntil();
        if (lu > millis())
            Serial.printf("[SYS] Lockdown: %lu ms kaldi\n", lu - millis());

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


// ══════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    delay(2000);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

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
    xTaskCreatePinnedToCore(taskRFID,     "rfid",     4096,  NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskSecurity, "security", 12288, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskCloud,    "cloud",    12288, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskWifi,     "wifi",     4096,  NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskSystem,   "system",   2048,  NULL, 1, NULL, 0);

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
