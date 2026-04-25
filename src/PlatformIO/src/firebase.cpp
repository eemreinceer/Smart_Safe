#include "firebase.h"
#include "config.h"
#include "systemstate.h"

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// ─────────────────────────────────────────────
//  Firebase Nesneleri
// ─────────────────────────────────────────────
FirebaseData   fbdo;
FirebaseData   fbdoStream;
FirebaseAuth   auth;
FirebaseConfig firebaseConfig;

static bool firebaseReady = false;


// ══════════════════════════════════════════════
//  NTP ZAMAN SENKRONİZASYONU
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
    {
        Serial.println("\n[NTP] ❌ Zaman senkronize edilemedi!");
        return;
    }

    Serial.println("\n[NTP] ✅ Zaman senkronize edildi");
    Serial.printf("[NTP] 📅 %04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec);
}


String getTimestamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "UNKNOWN_" + String(millis());
    }

    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    return String(buf);
}


// ══════════════════════════════════════════════
//  Firebase Yapı Oluşturma
// ══════════════════════════════════════════════
void initFirebaseStructure()
{
    Firebase.RTDB.setString(&fbdo, "/alarmControl", "IDLE");

    FirebaseJson config;
    config.set("alarmEnabled",      true);
    config.set("lockTimeout",       30);
    config.set("maxFailAttempts",   MAX_FAIL_ATTEMPTS);
    config.set("lockdownDuration",  LOCKDOWN_DURATION_MS / 1000);
    config.set("lockOpenDuration",  LOCK_OPEN_DURATION_MS / 1000);
    Firebase.RTDB.setJSON(&fbdo, "/config", &config);

    FirebaseJson device;
    device.set("deviceId",        DEVICE_ID);
    device.set("firmwareVersion", FIRMWARE_VER);
    device.set("simulationMode",  (bool)SIMULATION_MODE);
    device.set("ip",              WiFi.localIP().toString());
    device.set("lastBoot",        getTimestamp());
    device.set("state",           getStateName(currentState));
    device.set("online",          true);
    Firebase.RTDB.setJSON(&fbdo, "/device", &device);

    Serial.println("[FB] ✅ Firebase yapılandırması oluşturuldu");
}


// ══════════════════════════════════════════════
//  Cihaz Durumu Güncelle
// ══════════════════════════════════════════════
bool updateDeviceStatus()
{
    if (!firebaseReady || !Firebase.ready())
        return false;

    FirebaseJson json;
    json.set("state",       getStateName(currentState));
    json.set("lastUpdate",  getTimestamp());
    json.set("online",      true);
    json.set("ip",          WiFi.localIP().toString());
    json.set("freeHeap",    (int)ESP.getFreeHeap());
    json.set("uptime",      (int)(millis() / 1000));

    if (Firebase.RTDB.updateNode(&fbdo, "/device", &json))
    {
        Serial.println("[FB] ✅ Cihaz durumu güncellendi");
        return true;
    }

    Serial.println("[FB] ❌ Cihaz durumu güncellenemedi");
    return false;
}


// ══════════════════════════════════════════════
//  Firebase Başlatma
// ══════════════════════════════════════════════
void initFirebase()
{
    Serial.println("[FB] WiFi bağlanıyor...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 40)
    {
        delay(500);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\n[FB] ❌ WiFi bağlanamadı! Offline modda devam.");
        return;
    }

    Serial.println("\n[FB] ✅ WiFi bağlandı: " + WiFi.localIP().toString());

    // NTP başlat
    initNTP();

    // Firebase yapılandırma
    firebaseConfig.api_key      = API_KEY;
    firebaseConfig.database_url = DATABASE_URL;

    // Auth
    auth.user.email    = FIREBASE_AUTH_EMAIL;
    auth.user.password = FIREBASE_AUTH_PASSWORD;

    Firebase.begin(&firebaseConfig, &auth);
    Firebase.reconnectWiFi(true);

    fbdo.setResponseSize(4096);
    fbdoStream.setResponseSize(4096);

    firebaseReady = true;

    Serial.println("[FB] ✅ Firebase hazır");

    // İlk yapılandırma
    #if AUTO_INIT_FIREBASE
    initFirebaseStructure();
    #endif

    // İlk durum güncellemesi
    updateDeviceStatus();
}


// ══════════════════════════════════════════════
//  Log Gönder
// ══════════════════════════════════════════════
bool sendLog(String status, String id)
{
    if (!firebaseReady || !Firebase.ready())
        return false;

    FirebaseJson json;
    json.set("status",      status);
    json.set("userId",      id);
    json.set("timestamp",   getTimestamp());
    json.set("deviceId",    DEVICE_ID);
    json.set("state",       getStateName(currentState));

    #if SIMULATION_MODE
    json.set("source", "simulation");
    #else
    json.set("source", "hardware");
    #endif

    if (Firebase.RTDB.pushJSON(&fbdo, "/logs", &json))
    {
        Serial.println("[FB] ✅ Log gönderildi");
        updateDeviceStatus();
        return true;
    }

    Serial.println("[FB] ❌ Log gönderilemedi: " + fbdo.errorReason());
    return false;
}


// ══════════════════════════════════════════════
//  Alarm Tetikleyici Kontrolü
// ══════════════════════════════════════════════
bool checkAlarmTrigger()
{
    if (!firebaseReady || !Firebase.ready())
        return false;

    if (Firebase.RTDB.getString(&fbdo, "/alarmControl"))
    {
        String val = fbdo.stringData();

        if (val == "TRIGGER")
        {
            Firebase.RTDB.setString(&fbdo, "/alarmControl", "IDLE");
            return true;
        }
    }

    return false;
}


// ══════════════════════════════════════════════
//  Remote Config
// ══════════════════════════════════════════════
bool getRemoteConfig(bool &alarmEnabled, int &lockTimeout)
{
    if (!firebaseReady || !Firebase.ready())
        return false;

    if (Firebase.RTDB.getJSON(&fbdo, "/config"))
    {
        FirebaseJson    &json = fbdo.jsonObject();
        FirebaseJsonData result;

        json.get(result, "alarmEnabled");
        if (result.success)
            alarmEnabled = result.boolValue;

        json.get(result, "lockTimeout");
        if (result.success)
            lockTimeout = result.intValue;

        return true;
    }

    return false;
}


// ══════════════════════════════════════════════
//  Fotoğraf Yükle
// ══════════════════════════════════════════════
void uploadPhoto(uint8_t* buf, size_t len)
{
    if (!firebaseReady || !Firebase.ready())
    {
        Serial.println("[FB] ❌ Fotoğraf yüklenemedi, Firebase hazır değil");
        return;
    }

    #if SIMULATION_MODE

    FirebaseJson json;
    json.set("imageUrl",   "simulation://no-image");
    json.set("timestamp",  getTimestamp());
    json.set("deviceId",   DEVICE_ID);
    json.set("state",      getStateName(currentState));
    json.set("note",       "Simulation mode - no real image");

    Firebase.RTDB.pushJSON(&fbdo, "/photos", &json);
    Serial.println("[FB] 📸 Simülasyon fotoğraf kaydı gönderildi");

    #else

    Serial.printf("[FB] 📸 Fotoğraf yükleniyor... (%d bytes)\n", len);

    FirebaseJson json;
    json.set("size",       (int)len);
    json.set("timestamp",  getTimestamp());
    json.set("deviceId",   DEVICE_ID);
    json.set("state",      getStateName(currentState));

    if (Firebase.RTDB.pushJSON(&fbdo, "/photos", &json))
    {
        Serial.println("[FB] ✅ Fotoğraf kaydı eklendi");
    }
    else
    {
        Serial.println("[FB] ❌ Fotoğraf yüklenemedi: " + fbdo.errorReason());
    }

    #endif
}