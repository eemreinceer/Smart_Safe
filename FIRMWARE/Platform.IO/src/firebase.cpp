// Firebase REST istemcisi — sadece HTTP/REST sorumluluğu
// WiFi bağlantısı ve NTP: wifi_manager modülü tarafından yönetilir.

#include "firebase.h"
#include "wifi_manager.h"
#include "config.h"
#include "systemstate.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

static bool              firebaseReady = false;
static const char*       DB_HOST       = DATABASE_URL;
static SemaphoreHandle_t restMutex     = NULL;

// ══════════════════════════════════════════════
//  Zaman damgası (dahili yardımcı)
// ══════════════════════════════════════════════
static String getTimestamp()
{
    struct tm ti;
    if (!getLocalTime(&ti))
        return "UNKNOWN_" + String(millis());

    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ti);
    return String(buf);
}

static String   idToken             = "";
static uint32_t tokenExpirationTime = 0;

// ══════════════════════════════════════════════
//  Firebase Giriş (Auth REST API)
// ══════════════════════════════════════════════
static bool loginFirebase()
{
    if (!wifiIsConnected())
    {
        Serial.println("[FB-AUTH] WiFi bagli degil");
        return false;
    }

    // Kritik heap kontrolü — SSL ~40 KB ister
    if (ESP.getFreeHeap() < 60000)
    {
        Serial.printf("[FB-AUTH] Heap kritik (%u), giris iptal\n", ESP.getFreeHeap());
        return false;
    }

    Serial.println("[FB-AUTH] Firebase auth deneniyor...");
    Serial.printf("[FB-AUTH] Email: %s\n", FIREBASE_AUTH_EMAIL);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10);

    HTTPClient http;
    String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + String(API_KEY);

    if (!http.begin(client, url))
    {
        Serial.println("[FB-AUTH] http.begin() basarisiz");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);

    String body = "{\"email\":\"" + String(FIREBASE_AUTH_EMAIL) + "\",\"password\":\"" + String(FIREBASE_AUTH_PASSWORD) + "\",\"returnSecureToken\":true}";
    
    Serial.println("[FB-AUTH] POST gonderiliyor...");
    int code = http.POST(body);
    String response = http.getString();
    http.end();

    Serial.printf("[FB-AUTH] Yanit: HTTP %d\n", code);

    if (code == 200)
    {
        // Firebase bazen "idToken":"..." bazen "idToken": "..." döndürür
        int tokenIdx = response.indexOf("\"idToken\":\"");
        if (tokenIdx < 0) tokenIdx = response.indexOf("\"idToken\": \"");
        if (tokenIdx >= 0)
        {
            tokenIdx = response.indexOf(':', tokenIdx) + 1;
            while (response[tokenIdx] == ' ' || response[tokenIdx] == '"') tokenIdx++;
            int endIdx = response.indexOf('"', tokenIdx);
            idToken = response.substring(tokenIdx, endIdx);

            int expiresIdx = response.indexOf("\"expiresIn\":\"");
            if (expiresIdx < 0) expiresIdx = response.indexOf("\"expiresIn\": \"");
            if (expiresIdx >= 0)
            {
                expiresIdx = response.indexOf(':', expiresIdx) + 1;
                while (response[expiresIdx] == ' ' || response[expiresIdx] == '"') expiresIdx++;
                int endExpiresIdx = response.indexOf('"', expiresIdx);
                int expiresIn = response.substring(expiresIdx, endExpiresIdx).toInt();
                tokenExpirationTime = time(nullptr) + expiresIn - 300;
            }
            else
            {
                tokenExpirationTime = time(nullptr) + 3600 - 300;
            }
            Serial.println("[FB-AUTH] Token alindi, giris basarili!");
            return true;
        }
    }

    Serial.printf("[FB-AUTH] HTTP %d: %s\n", code, response.c_str());
    return false;
}

// ══════════════════════════════════════════════
//  REST: PATCH / PUT / POST / GET
// ══════════════════════════════════════════════
static bool restRequest(const String& method,
                        const String& path,
                        const String& body,
                        String&       response)
{
    if (!wifiIsConnected()) return false;

    // Kritik heap kontrolü — SSL ~40 KB ister
    if (ESP.getFreeHeap() < 60000)
    {
        Serial.printf("[FB] Heap kritik (%u), istek iptal\n", ESP.getFreeHeap());
        return false;
    }

    if (restMutex && xSemaphoreTake(restMutex, pdMS_TO_TICKS(12000)) != pdTRUE)
    {
        Serial.println("[FB] Mutex timeout, istek atlandi");
        return false;
    }

    // Mutex sonrası tekrar kontrol
    if (ESP.getFreeHeap() < 60000)
    {
        Serial.printf("[FB] Post-mutex heap kritik (%u)\n", ESP.getFreeHeap());
        xSemaphoreGive(restMutex);
        return false;
    }

    // Oturum kontrolü ve token yenileme
    if (idToken == "" || time(nullptr) >= tokenExpirationTime)
    {
        Serial.println("[FB-AUTH] Token yok/suresi dolmus, giris yapiliyor...");
        if (!loginFirebase())
        {
            Serial.println("[FB-AUTH] Yeniden giris basarisiz, istek iptal");
            xSemaphoreGive(restMutex);
            return false;
        }
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10); // 10 sn TCP timeout — SSL donma önlemi

    HTTPClient http;
    String url = String(DB_HOST) + path + ".json";
    if (idToken != "")
    {
        url += "?auth=" + idToken;
    }

    if (!http.begin(client, url))
    {
        Serial.println("[FB] http.begin() basarisiz");
        xSemaphoreGive(restMutex);
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);

    int code;
    if      (method == "PATCH") code = http.PATCH(body);
    else if (method == "PUT")   code = http.PUT(body);
    else if (method == "POST")  code = http.POST(body);
    else                        code = http.GET();

    response = http.getString();
    http.end();
    xSemaphoreGive(restMutex);

    if (code >= 200 && code < 300) return true;

    Serial.printf("[FB] HTTP %d: %s\n", code, response.c_str());
    return false;
}

// ══════════════════════════════════════════════
//  Firebase başlangıç yapısı
// ══════════════════════════════════════════════
void initFirebaseStructure()
{
    String resp;
    String body =
        "{\"config\":{"
            "\"alarmEnabled\":true,"
            "\"lockTimeout\":30,"
            "\"maxFailAttempts\":" + String(MAX_FAIL_ATTEMPTS) + ","
            "\"lockdownDuration\":" + String(LOCKDOWN_DURATION_MS / 1000) + ","
            "\"lockOpenDuration\":" + String(LOCK_OPEN_DURATION_MS / 1000) +
        "},\"control\":{\"alarm\":\"IDLE\"}}";

    if (restRequest("PATCH", "/safe_001", body, resp))
        Serial.println("[FB] Config yazildi");
    else
        Serial.println("[FB] Config yazilamadi");
}

// ══════════════════════════════════════════════
//  Firebase başlat
//  WiFi + NTP artık wifi_manager sorumluluğunda.
// ══════════════════════════════════════════════
void initFirebase()
{
    if (restMutex == NULL)
        restMutex = xSemaphoreCreateMutex();

    if (!wifiIsConnected())
    {
        Serial.println("[FB] WiFi yok, offline modda.");
        return;
    }

    firebaseReady = true;
    Serial.println("[FB] Firebase REST hazir");

    #if AUTO_INIT_FIREBASE
    initFirebaseStructure();
    #endif

    updateDeviceStatus();
}

// ══════════════════════════════════════════════
//  Cihaz durumu
// ══════════════════════════════════════════════
bool updateDeviceStatus()
{
    if (!firebaseReady) return false;

    String body =
        "{\"is_online\":true,"
        "\"is_locked\":true,"
        "\"last_seen\":"  + String((int)time(nullptr)) + ","
        "\"ip\":\""       + WiFi.localIP().toString()  + "\","
        "\"freeHeap\":"   + String((int)ESP.getFreeHeap()) + ","
        "\"state\":\""    + String(getStateName(currentState)) + "\","
        "\"firmware\":\"" + String(FIRMWARE_VER) + "\"}";

    String resp;
    if (restRequest("PATCH", "/safe_001/status", body, resp))
        return true;

    Serial.println("[FB] Durum guncellenemedi");
    return false;
}

// ══════════════════════════════════════════════
//  Kilit durumu (anlık)
// ══════════════════════════════════════════════
bool updateLockState(bool isLocked)
{
    if (!firebaseReady) return false;

    String body =
        "{\"is_locked\":"  + String(isLocked ? "true" : "false") + ","
        "\"last_seen\":"   + String((int)time(nullptr)) + ","
        "\"state\":\""     + String(getStateName(currentState)) + "\"}";

    String resp;
    if (restRequest("PATCH", "/safe_001/status", body, resp))
    {
        Serial.printf("[FB] Kilit: %s\n", isLocked ? "KILITLI" : "ACIK");
        return true;
    }
    return false;
}

// ══════════════════════════════════════════════
//  Log gönder
// ══════════════════════════════════════════════
bool sendLog(String status, String id, String photoBase64)
{
    if (!firebaseReady) return false;

    String body =
        "{\"event\":\""  + status + "\","
        "\"method\":\"" + id     + "\","
        "\"timestamp\":" + String((int)time(nullptr)) + ","
        "\"state\":\""   + String(getStateName(currentState)) + "\"";

    if (photoBase64.length() > 0)
        body += ",\"photo_base64\":\"" + photoBase64 + "\"";

    body += "}";

    String resp;
    if (restRequest("POST", "/safe_001/logs", body, resp))
    {
        Serial.println("[FB] Log gonderildi");
        return true;
    }
    Serial.println("[FB] Log gonderilemedi");
    return false;
}

// ══════════════════════════════════════════════
//  Uzak komut kontrolü — tek SSL ile alarm + unlock
// ══════════════════════════════════════════════
bool checkRemoteCommands(bool& alarmTrigger, bool& remoteUnlock)
{
    alarmTrigger = false;
    remoteUnlock = false;

    if (!firebaseReady) return false;

    String resp;
    if (!restRequest("GET", "/safe_001/control/alarm", "", resp))
        return false;

    alarmTrigger = (resp.indexOf("TRIGGER")       >= 0);
    remoteUnlock = (resp.indexOf("REMOTE_UNLOCK")  >= 0);

    if (alarmTrigger || remoteUnlock)
        restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp);

    return true;
}

// ══════════════════════════════════════════════
//  Stream (no-op — REST modunda yok)
// ══════════════════════════════════════════════
void startLockStatusStream()
{
    Serial.println("[FB] Stream REST modunda devre disi");
}
