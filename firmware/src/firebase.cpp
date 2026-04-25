// REST API tabanli Firebase istemcisi
// mobizt kutuphanesi Wokwi'de hang ediyor, bu yuzden HTTPClient ile REST kullaniyoruz.
// Rules acik olmali (test): {".read": true, ".write": true}

#include "firebase.h"
#include "config.h"
#include "systemstate.h"
#include "lock.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

static bool firebaseReady = false;
static const char* DB_HOST = DATABASE_URL;  // ornegin https://xxx.firebaseio.com
static SemaphoreHandle_t restMutex = NULL;

// ══════════════════════════════════════════════
//  NTP
// ══════════════════════════════════════════════
void initNTP()
{
    configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT, NTP_SERVER);
    Serial.print("[NTP] Zaman senkronize ediliyor");

    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 20)
    {
        Serial.print(".");
        delay(500);
        retry++;
    }

    if (retry >= 20)
        Serial.println("\n[NTP] Zaman senkronize edilemedi");
    else
        Serial.println("\n[NTP] Zaman senkronize edildi");
}

String getTimestamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
        return "UNKNOWN_" + String(millis());

    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    return String(buf);
}

// ══════════════════════════════════════════════
//  REST: PATCH/PUT/POST/GET
// ══════════════════════════════════════════════
static bool restRequest(const String& method, const String& path, const String& body, String& response)
{
    if (WiFi.status() != WL_CONNECTED)
        return false;

    // Sadece tek SSL bagi ayni anda - heap'i koru
    if (restMutex && xSemaphoreTake(restMutex, pdMS_TO_TICKS(15000)) != pdTRUE)
    {
        Serial.println("[FB] Mutex timeout, istek atlandi");
        return false;
    }

    // Heap dusukse bekle
    if (ESP.getFreeHeap() < 70000)
    {
        Serial.printf("[FB] Heap dusuk (%u), bekleniyor\n", ESP.getFreeHeap());
        delay(1500);
    }

    WiFiClientSecure client;
    client.setInsecure();   // Wokwi'de sertifika dogrulamasi yok

    HTTPClient http;
    String url = String(DB_HOST) + path + ".json";

    if (!http.begin(client, url))
    {
        Serial.println("[FB] http.begin() basarisiz");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);

    int code;
    if (method == "PATCH")
        code = http.PATCH(body);
    else if (method == "PUT")
        code = http.PUT(body);
    else if (method == "POST")
        code = http.POST(body);
    else
        code = http.GET();

    response = http.getString();
    http.end();

    if (restMutex)
        xSemaphoreGive(restMutex);

    if (code >= 200 && code < 300)
        return true;

    Serial.printf("[FB] HTTP %d: %s\n", code, response.c_str());
    return false;
}

// ══════════════════════════════════════════════
//  Firebase yapilandirma
// ══════════════════════════════════════════════
void initFirebaseStructure()
{
    String resp;
    String body = "{"
        "\"alarmEnabled\":true,"
        "\"lockTimeout\":30,"
        "\"maxFailAttempts\":" + String(MAX_FAIL_ATTEMPTS) + ","
        "\"lockdownDuration\":" + String(LOCKDOWN_DURATION_MS / 1000) + ","
        "\"lockOpenDuration\":" + String(LOCK_OPEN_DURATION_MS / 1000) + "}";

    if (restRequest("PATCH", "/safe_001/config", body, resp))
        Serial.println("[FB] Config yazildi");
    else
        Serial.println("[FB] Config yazilamadi");

    if (restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp))
        Serial.println("[FB] Control yazildi");
}

// ══════════════════════════════════════════════
//  Cihaz durumu
// ══════════════════════════════════════════════
bool updateDeviceStatus()
{
    if (!firebaseReady)
        return false;

    String body = "{"
        "\"is_online\":true,"
        "\"is_locked\":true,"
        "\"last_seen\":" + String((int)time(nullptr)) + ","
        "\"ip\":\"" + WiFi.localIP().toString() + "\","
        "\"freeHeap\":" + String((int)ESP.getFreeHeap()) + ","
        "\"state\":\"" + String(getStateName(currentState)) + "\","
        "\"firmware\":\"" + String(FIRMWARE_VER) + "\""
        "}";

    String resp;
    if (restRequest("PATCH", "/safe_001/status", body, resp))
    {
        Serial.println("[FB] Cihaz durumu guncellendi");
        return true;
    }
    Serial.println("[FB] Cihaz durumu guncellenemedi");
    return false;
}

// ══════════════════════════════════════════════
//  Kilit durumunu push et (anlik)
// ══════════════════════════════════════════════
bool updateLockState(bool isLocked)
{
    if (!firebaseReady)
        return false;

    String body = "{"
        "\"is_locked\":" + String(isLocked ? "true" : "false") + ","
        "\"last_seen\":" + String((int)time(nullptr)) + ","
        "\"state\":\"" + String(getStateName(currentState)) + "\""
        "}";

    String resp;
    if (restRequest("PATCH", "/safe_001/status", body, resp))
    {
        Serial.printf("[FB] Kilit durumu: %s\n", isLocked ? "KILITLI" : "ACIK");
        return true;
    }
    return false;
}

// ══════════════════════════════════════════════
//  Firebase baslat
// ══════════════════════════════════════════════
void initFirebase()
{
    if (restMutex == NULL)
        restMutex = xSemaphoreCreateMutex();

    Serial.println("[FB] WiFi baglaniyor...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 120)
    {
        delay(500);
        Serial.print(".");
        retry++;
        if (retry % 20 == 0)
            Serial.printf("\n[FB] Bekleniyor... status=%d\n", WiFi.status());
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\n[FB] WiFi baglanamadi! Offline modda devam.");
        return;
    }

    Serial.println("\n[FB] WiFi baglandi: " + WiFi.localIP().toString());

    initNTP();

    firebaseReady = true;
    Serial.println("[FB] Firebase REST hazir");

    #if AUTO_INIT_FIREBASE
    initFirebaseStructure();
    #endif

    updateDeviceStatus();
}

// ══════════════════════════════════════════════
//  Log gonder
// ══════════════════════════════════════════════
bool sendLog(String status, String id)
{
    if (!firebaseReady)
        return false;

    String body = "{"
        "\"event\":\"" + status + "\","
        "\"method\":\"" + id + "\","
        "\"timestamp\":" + String((int)time(nullptr)) + ","
        "\"state\":\"" + String(getStateName(currentState)) + "\""
        "}";

    String resp;
    if (restRequest("POST", "/safe_001/logs", body, resp))
    {
        Serial.println("[FB] Log gonderildi");
        updateDeviceStatus();
        return true;
    }
    Serial.println("[FB] Log gonderilemedi");
    return false;
}

// ══════════════════════════════════════════════
//  Alarm kontrol
// ══════════════════════════════════════════════
bool checkAlarmTrigger()
{
    if (!firebaseReady)
        return false;

    String resp;
    if (!restRequest("GET", "/safe_001/control/alarm", "", resp))
        return false;

    if (resp.indexOf("TRIGGER") >= 0)
    {
        restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp);
        return true;
    }
    return false;
}

bool checkRemoteUnlock()
{
    if (!firebaseReady)
        return false;

    String resp;
    if (!restRequest("GET", "/safe_001/control/alarm", "", resp))
        return false;

    if (resp.indexOf("REMOTE_UNLOCK") >= 0)
    {
        restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp);
        return true;
    }
    return false;
}

// ══════════════════════════════════════════════
//  Remote config
// ══════════════════════════════════════════════
bool getRemoteConfig(bool &alarmEnabled, int &lockTimeout)
{
    if (!firebaseReady)
        return false;

    String resp;
    if (!restRequest("GET", "/safe_001/config", "", resp))
        return false;

    if (resp.indexOf("\"alarmEnabled\":true") >= 0)
        alarmEnabled = true;
    else if (resp.indexOf("\"alarmEnabled\":false") >= 0)
        alarmEnabled = false;

    int idx = resp.indexOf("\"lockTimeout\":");
    if (idx >= 0)
        lockTimeout = resp.substring(idx + 14).toInt();

    return true;
}

// ══════════════════════════════════════════════
//  Fotograf yukleme (simulasyonda placeholder)
// ══════════════════════════════════════════════
void uploadPhoto(uint8_t* buf, size_t len)
{
    if (!firebaseReady)
        return;

    String body = "{"
        "\"imageUrl\":\"simulation://no-image\","
        "\"timestamp\":" + String((int)time(nullptr)) + ","
        "\"state\":\"" + String(getStateName(currentState)) + "\","
        "\"note\":\"Simulation mode\""
        "}";

    String resp;
    if (restRequest("POST", "/safe_001/photos", body, resp))
        Serial.println("[FB] Foto kaydi gonderildi");
}

// ══════════════════════════════════════════════
//  Stream (REST'te yok, no-op)
// ══════════════════════════════════════════════
void startLockStatusStream()
{
    Serial.println("[FB] Stream REST modunda devre disi");
}
