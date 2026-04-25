#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "fingerprint.h"
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
//  Event Tipleri
// ─────────────────────────────────────────────
#define EVENT_AUTHORIZED   1
#define EVENT_UNAUTHORIZED 2

// ─────────────────────────────────────────────
//  Event Struct
// ─────────────────────────────────────────────
struct SafeEvent
{
    int  type;
    char id[32];
};

// ─────────────────────────────────────────────
//  Global Değişkenler
// ─────────────────────────────────────────────
QueueHandle_t eventQueue;

volatile int           failCount = 0;
volatile unsigned long lockUntil = 0;

// ─────────────────────────────────────────────
//  Forward Declarations
// ─────────────────────────────────────────────
void taskFingerprint(void *pv);
void taskSecurity   (void *pv);
void taskCloud      (void *pv);
void taskWifi       (void *pv);
void taskSystem     (void *pv);
void taskConfig     (void *pv);


// ══════════════════════════════════════════════
//  TASK 1 — Fingerprint
//  Core : 1
// ══════════════════════════════════════════════
void taskFingerprint(void *pv)
{
    while (true)
    {
        // Lockdown aktifse sensörü okuma
        if (millis() < lockUntil)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // State kontrol
        if (currentState == STATE_LOCKDOWN ||
            currentState == STATE_ALARM)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int fingerID = readFingerprint();

        SafeEvent event;
        memset(&event, 0, sizeof(event));

        if (fingerID > 0)
        {
            event.type = EVENT_AUTHORIZED;
            snprintf(event.id, sizeof(event.id), "%d", fingerID);
            setState(STATE_AUTHORIZED);
            xQueueSend(eventQueue, &event, portMAX_DELAY);
        }
        else if (fingerID == -2)
        {
            event.type = EVENT_UNAUTHORIZED;
            snprintf(event.id, sizeof(event.id), "UNKNOWN");
            setState(STATE_UNAUTHORIZED);
            xQueueSend(eventQueue, &event, portMAX_DELAY);
        }
        else
        {
            // Boşta, idle state
            if (currentState != STATE_LOCKDOWN &&
                currentState != STATE_ALARM)
            {
                setState(STATE_IDLE);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


// ══════════════════════════════════════════════
//  TASK 2 — Security
//  Core : 1
// ══════════════════════════════════════════════
void taskSecurity(void *pv)
{
    SafeEvent event;

    while (true)
    {
        // Queue'dan event bekle
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY) != pdTRUE)
            continue;

        // Lockdown kontrolü
        if (millis() < lockUntil)
        {
            Serial.println("[SECURITY] Lockdown aktif, event reddedildi.");
            setState(STATE_LOCKDOWN);
            continue;
        }

        // ──────────────────────────────────────
        //  SENARYO A : YETKİLİ GİRİŞ
        // ──────────────────────────────────────
        if (event.type == EVENT_AUTHORIZED)
        {
            Serial.printf("[SECURITY] ✅ Yetkili giriş → ID: %s\n", event.id);

            setState(STATE_AUTHORIZED);

            unlockSafe();

            if (!sendLog("AUTHORIZED", String(event.id)))
                storeOfflineLog("AUTHORIZED", String(event.id));

            logEvent(String("AUTHORIZED USER ") + event.id);

            failCount = 0;

            setState(STATE_IDLE);
        }

        // ──────────────────────────────────────
        //  SENARYO B : YETKİSİZ GİRİŞ
        // ──────────────────────────────────────
        else if (event.type == EVENT_UNAUTHORIZED)
        {
            Serial.println("[SECURITY] ❌ YETKİSİZ ERİŞİM!");

            setState(STATE_UNAUTHORIZED);

            // Fotoğraf çek ve Storage'a yükle
            capturePhoto();

            // Firebase'e log gönder
            if (!sendLog("UNAUTHORIZED", String(event.id)))
                storeOfflineLog("UNAUTHORIZED", String(event.id));

            logEvent("UNAUTHORIZED ACCESS");

            failCount++;

            Serial.printf("[SECURITY] Başarısız deneme: %d/%d\n",
                          failCount, MAX_FAIL_ATTEMPTS);

            // Lockdown kontrolü
            if (failCount >= MAX_FAIL_ATTEMPTS)
            {
                lockUntil = millis() + LOCKDOWN_DURATION_MS;
                failCount = 0;

                setState(STATE_LOCKDOWN);

                Serial.println("[SECURITY] ⚠️ LOCKDOWN AKTİF!");
                logEvent("LOCKDOWN ACTIVATED");
            }
            else
            {
                setState(STATE_IDLE);
            }
        }
    }
}


// ══════════════════════════════════════════════
//  TASK 3 — Cloud
//  Core : 0
// ══════════════════════════════════════════════
void taskCloud(void *pv)
{
    while (true)
    {
        handleOTA();

        // WiFi varsa offline logları sync et
        if (WiFi.status() == WL_CONNECTED)
            trySyncOfflineLogs();

        // Firebase'den alarm komutu geldi mi?
        if (checkAlarmTrigger())
        {
            Serial.println("[CLOUD] 🚨 Alarm komutu alındı!");
            setState(STATE_ALARM);
            triggerAlarm();
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// ══════════════════════════════════════════════
//  TASK 4 — WiFi Monitor
//  Core : 0
// ══════════════════════════════════════════════
void taskWifi(void *pv)
{
    while (true)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("[WiFi] Bağlantı koptu, yeniden bağlanıyor...");

            WiFi.disconnect();
            vTaskDelay(pdMS_TO_TICKS(1000));
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

            int retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 20)
            {
                vTaskDelay(pdMS_TO_TICKS(500));
                retry++;
                Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED)
                Serial.println("\n[WiFi] ✅ Bağlandı");
            else
                Serial.println("\n[WiFi] ❌ Bağlantı başarısız");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


// ══════════════════════════════════════════════
//  TASK 5 — System Monitor
//  Core : 0
// ══════════════════════════════════════════════
void taskSystem(void *pv)
{
    while (true)
    {
        Serial.printf("[SYS] Heap: %u bytes | State: %s\n",
                      ESP.getFreeHeap(),
                      getStateName(currentState));

        if (lockUntil > millis())
        {
            Serial.printf("[SYS] Lockdown: %lu ms kaldı\n",
                          lockUntil - millis());
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


// ══════════════════════════════════════════════
//  TASK 6 — Remote Config
//  Core : 0
// ══════════════════════════════════════════════
void taskConfig(void *pv)
{
    bool alarmEnabled = true;
    int  lockTimeout  = 30;

    while (true)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (getRemoteConfig(alarmEnabled, lockTimeout))
            {
                Serial.printf("[CFG] alarm:%d timeout:%d\n",
                              alarmEnabled, lockTimeout);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


// ══════════════════════════════════════════════
//  SETUP
// ══════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.println("\n[SETUP] Akıllı Güvenlik Kasası başlatılıyor...");

    #if SIMULATION_MODE
    Serial.println("[SETUP] ⚠️ SİMÜLASYON MODU AKTİF");
    #endif

    // Modül başlatmaları
    initLogger();
    initOfflineQueue();
    initFingerprint();
    initCamera();
    initLock();
    initAlarm();
    initFirebase();
    initOTA();

    // State machine başlat
    setState(STATE_IDLE);

    // Watchdog
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    // Event queue
    eventQueue = xQueueCreate(10, sizeof(SafeEvent));
    if (eventQueue == NULL)
    {
        Serial.println("[SETUP] ❌ Queue oluşturulamadı! Sistem durdu.");
        while (true);
    }

    // Task'leri başlat
    xTaskCreatePinnedToCore(taskFingerprint, "finger",   4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskSecurity,    "security", 6144, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskCloud,       "cloud",    8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskWifi,        "wifi",     4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskSystem,      "system",   2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskConfig,      "config",   4096, NULL, 1, NULL, 0);

    Serial.println("[SETUP] ✅ SMART SAFE READY\n");
}


// ══════════════════════════════════════════════
//  LOOP
// ══════════════════════════════════════════════
void loop()
{
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(1000));
}