// Firebase REST istemcisi — sadece HTTP/REST sorumluluğu
// WiFi bağlantısı ve NTP: wifi_manager modülü tarafından yönetilir.

#include "firebase.h"
#include "wifi_manager.h"
#include "config.h"
#include "systemstate.h"
#include "lock.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

static bool              firebaseReady = false;
static const char*       DB_HOST       = DATABASE_URL;
static SemaphoreHandle_t restMutex     = NULL;

static bool hasFirebaseConfig()
{
    return strlen(API_KEY) > 0 && strlen(DATABASE_URL) > 0 &&
           strlen(FIREBASE_AUTH_EMAIL) > 0 &&
           strlen(FIREBASE_AUTH_PASSWORD) > 0 &&
           strlen(FIREBASE_ROOT_CA_PEM) > 0;
}

static String   idToken             = "";
static uint32_t tokenExpirationMillis = 0;

static String jsonEscape(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8);
    for (size_t i = 0; i < value.length(); ++i)
    {
        const char c = value[i];
        if (c == '"' || c == '\\') escaped += '\\';
        if (static_cast<uint8_t>(c) < 0x20)
        {
            char encoded[7];
            snprintf(encoded, sizeof(encoded), "\\u%04X", static_cast<unsigned char>(c));
            escaped += encoded;
        }
        else
        {
            escaped += c;
        }
    }
    return escaped;
}

// ══════════════════════════════════════════════
//  Firebase Giriş (Auth REST API)
// ══════════════════════════════════════════════
static bool loginFirebase()
{
    if (!hasFirebaseConfig())
    {
        Serial.println("[FB-AUTH] Eksik Firebase/TLS yapilandirmasi");
        return false;
    }

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

    WiFiClientSecure client;
    client.setCACert(FIREBASE_ROOT_CA_PEM);
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

    String body = "{\"email\":\"" + jsonEscape(FIREBASE_AUTH_EMAIL) + "\",\"password\":\"" + jsonEscape(FIREBASE_AUTH_PASSWORD) + "\",\"returnSecureToken\":true}";
    
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
                tokenExpirationMillis = millis() + static_cast<uint32_t>(max(60, expiresIn - 300)) * 1000UL;
            }
            else
            {
                tokenExpirationMillis = millis() + 3300UL * 1000UL;
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

    if (!restMutex)
    {
        Serial.println("[FB] REST mutex yok; istek reddedildi");
        return false;
    }

    if (xSemaphoreTake(restMutex, pdMS_TO_TICKS(12000)) != pdTRUE)
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
    if (idToken == "" || static_cast<long>(millis() - tokenExpirationMillis) >= 0)
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
    client.setCACert(FIREBASE_ROOT_CA_PEM);
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
    if (restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp))
        Serial.println("[FB] Komut kanali guvenli duruma getirildi");
    else
        Serial.println("[FB] Komut kanali baslatilamadi");
}

// ══════════════════════════════════════════════
//  Firebase başlat
//  WiFi + NTP artık wifi_manager sorumluluğunda.
// ══════════════════════════════════════════════
void initFirebase()
{
    if (restMutex == NULL)
        restMutex = xSemaphoreCreateMutex();

    if (restMutex == NULL)
    {
        Serial.println("[FB] REST mutex olusturulamadi; cloud devre disi");
        firebaseReady = false;
        return;
    }

    if (!hasFirebaseConfig())
    {
        Serial.println("[FB] Firebase veya CA tanimsiz; cloud fail-closed devre disi.");
        return;
    }

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

bool firebaseIsReady()
{
    return firebaseReady;
}

bool firebaseIsConfigured()
{
    return hasFirebaseConfig();
}

// ══════════════════════════════════════════════
//  Cihaz durumu
// ══════════════════════════════════════════════
bool updateDeviceStatus()
{
    if (!firebaseReady) return false;

    String body =
        "{\"is_online\":true,"
        "\"is_locked\":" + String(isSafeLocked() ? "true" : "false") + ","
        "\"last_seen\":"  + String((int)time(nullptr)) + ","
        "\"ip\":\""       + WiFi.localIP().toString()  + "\","
        "\"freeHeap\":"   + String((int)ESP.getFreeHeap()) + ","
        "\"state\":\""    + String(getStateName(getState())) + "\","
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
        "\"state\":\""     + String(getStateName(getState())) + "\"}";

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
bool sendLog(String status, String id, String photoBase64, uint32_t eventTimestamp)
{
    if (!firebaseReady) return false;

    if (eventTimestamp == 0)
        eventTimestamp = static_cast<uint32_t>(time(nullptr));

    String body =
        "{\"event\":\""  + jsonEscape(status) + "\","
        "\"method\":\"" + jsonEscape(id)     + "\","
        "\"timestamp\":" + String(eventTimestamp) + ","
        "\"state\":\""   + String(getStateName(getState())) + "\"";

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
//  Uzak komut kontrolü — cloud yalnızca alarm tetikleyebilir
// ══════════════════════════════════════════════
bool checkRemoteCommands(bool& alarmTrigger)
{
    alarmTrigger = false;

    if (!firebaseReady) return false;

    String resp;
    if (!restRequest("GET", "/safe_001/control/alarm", "", resp))
        return false;

    resp.trim();
    alarmTrigger = (resp == "\"TRIGGER\"");

    if (alarmTrigger)
    {
        if (!restRequest("PUT", "/safe_001/control/alarm", "\"IDLE\"", resp))
        {
            alarmTrigger = false;
            Serial.println("[FB] Komut claim edilemedi; guvenli sekilde reddedildi");
            return false;
        }
    }

    return true;
}
