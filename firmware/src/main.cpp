#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "rfid.h"          // fingerprint.h yerine
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
void taskRFID    (void *pv);
void taskSecurity(void *pv);
void taskCloud   (void *pv);
void taskWifi    (void *pv);
void taskSystem  (void *pv);
void taskConfig  (void *pv);


// ══════════════════════════════════════════════
//  TASK 1 — RFID Okuyucu
//  Core : 1
// ══════════════════════════════════════════════
void taskRFID(void *pv)
{
    while (true)
    {
        // Lockdown aktifse okuma
        if (millis() < lockUntil)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (currentState == STATE_LOCKDOWN ||
            currentState == STATE_ALARM)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int cardID = readRFID();

        SafeEvent event;
        memset(&event, 0, sizeof(event));

        if (cardID > 0)
        {
            event.type = EVENT_AUTHORIZED;
            snprintf(event.id, sizeof(event.id), "KART-%d", cardID);
            setState(STATE_AUTHORIZED);
            xQueueSend(eventQueue, &event, portMAX_DELAY);
        }
        else if (cardID == -2)
        {
            event.type = EVENT_UNAUTHORIZED;
            snprintf(event.id, sizeof(event.id), "UNKNOWN");
            setState(STATE_UNAUTHORIZED);
            xQueueSend(eventQueue, &event, portMAX_DELAY);
        }
        else
        {
            // Kart yok → idle
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
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY) != pdTRUE)
            continue;

        if (millis() < lockUntil)
        {
            Serial.println("[SECURITY] Lockdown aktif, event reddedildi.");
            setState(STATE_LOCKDOWN);
            continue;
        }

        // ──────────────────────────────────────
        //  YETKİLİ GİRİŞ
        // ──────────────────────────────────────
        if (event.type == EVENT_AUTHORIZED)
        {
            Serial.printf("[SECURITY] ✅ Yetkili giriş → %s\n", event.id);

            setState(STATE_AUTHORIZED);

            updateLockState(false);   // Dashboard: kilit AÇIK
            unlockSafe();             // 3sn açık tutar, kapatır
            // Kapanma sonrası 1sn bekle ki SSL heap toparlansın
            vTaskDelay(pdMS_TO_TICKS(1000));

            // sendLog -> updateDeviceStatus (is_locked:true yazar)
            if (!sendLog("AUTHORIZED", String(event.id)))
                storeOfflineLog("AUTHORIZED", String(event.id));

            logEvent(String("AUTHORIZED ") + event.id);

            failCount = 0;
            setState(STATE_IDLE);
        }

        // ──────────────────────────────────────
        //  YETKİSİZ GİRİŞ
        // ──────────────────────────────────────
        else if (event.type == EVENT_UNAUTHORIZED)
        {
            Serial.println("[SECURITY] ❌ YETKİSİZ ERİŞİM!");

            setState(STATE_UNAUTHORIZED);
            capturePhoto();

            if (!sendLog("UNAUTHORIZED", String(event.id)))
                storeOfflineLog("UNAUTHORIZED", String(event.id));

            logEvent("UNAUTHORIZED ACCESS");

            failCount++;
            Serial.printf("[SECURITY] Başarısız deneme: %d/%d\n",
                          failCount, MAX_FAIL_ATTEMPTS);

            if (failCount >= MAX_FAIL_ATTEMPTS)
            {
                lockUntil = millis() + LOCKDOWN_DURATION_MS;
                failCount = 0;
                setState(STATE_LOCKDOWN);
                triggerAlarm();
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
    int counter = 0;
    while (true)
    {
        handleOTA();   // her saniye OTA kontrol

        // Alarm kontrolu ve offline sync 5 saniyede bir (SSL heap koru)
        if (counter % 5 == 0)
        {
            if (WiFi.status() == WL_CONNECTED)
                trySyncOfflineLogs();

            if (checkAlarmTrigger())
            {
                Serial.println("[CLOUD] 🚨 Uzaktan alarm komutu!");
                setState(STATE_ALARM);
                triggerAlarm();
            }

            if (checkRemoteUnlock())
            {
                Serial.println("[CLOUD] 🔓 Uzaktan kilit açma komutu!");
                SafeEvent ev;
                memset(&ev, 0, sizeof(ev));
                ev.type = EVENT_AUTHORIZED;
                snprintf(ev.id, sizeof(ev.id), "REMOTE");
                xQueueSend(eventQueue, &ev, portMAX_DELAY);
            }
        }
        counter++;
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
            Serial.printf("[SYS] Lockdown: %lu ms kaldı\n",
                          lockUntil - millis());

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
                Serial.printf("[CFG] alarm:%d timeout:%d\n",
                              alarmEnabled, lockTimeout);
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
    delay(2000);   // Serial konsolunun bağlanmasını bekle (Wokwi için)
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.println("\n\n========================================");
    Serial.println("[SETUP] Akilli Guvenlik Kasasi basliyor");
    Serial.println("========================================");

    #if SIMULATION_MODE
    Serial.println("[SETUP] ⚠️ SİMÜLASYON MODU AKTİF");
    Serial.println("[SETUP] Komut formatı: RFID:AABBCCDD");
    #endif

    initLogger();
    initOfflineQueue();
    initRFID();        // initFingerprint() yerine
    initCamera();
    initLock();
    initAlarm();
    initFirebase();
    initOTA();

    setState(STATE_IDLE);

    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);

    eventQueue = xQueueCreate(10, sizeof(SafeEvent));
    if (eventQueue == NULL)
    {
        Serial.println("[SETUP] ❌ Queue oluşturulamadı! Sistem durdu.");
        while (true);
    }

    xTaskCreatePinnedToCore(taskRFID,     "rfid",     4096,  NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskSecurity, "security", 8192,  NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskCloud,    "cloud",    12288, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskWifi,     "wifi",     4096,  NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskSystem,   "system",   2048,  NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(taskConfig,   "config",   12288, NULL, 1, NULL, 0);

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