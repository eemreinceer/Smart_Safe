#include "wifi_manager.h"
#include "config.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// ─── NTP (dahili) ────────────────────────────
static void syncNTP()
{
    configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT, NTP_SERVER);
    Serial.print("[NTP] Senkronize ediliyor");

    struct tm ti;
    int retry = 0;
    while (!getLocalTime(&ti) && retry < 20)
    {
        Serial.print(".");
        delay(500);
        retry++;
    }
    Serial.println(retry < 20 ? "\n[NTP] Tamam" : "\n[NTP] Basarisiz");
}

// ─── Ortak ayarlar ───────────────────────────
static void applySettings()
{
    WiFi.setTxPower(WIFI_POWER_17dBm); // kamera + WiFi akım zirvesini azalt
    WiFi.setSleep(false);
}

// ─── İlk bağlantı (setup'ta, bloklar) ───────
void wifiInit(int maxRetries)
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(false);

    Serial.print("[WiFi] Baglaniyor");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < maxRetries)
    {
        delay(500);
        Serial.print(".");
        retry++;
        if (retry % 20 == 0)
            Serial.printf("\n[WiFi] status=%d\n", WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        applySettings();
        Serial.println("\n[WiFi] Baglandi: " + WiFi.localIP().toString());
        syncNTP();
    }
    else
    {
        Serial.println("\n[WiFi] Baglanamadi, offline modda devam.");
    }
}

// ─── Yeniden bağlan (taskWifi'den çağrılır) ─
void wifiReconnect()
{
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.println("[WiFi] Baglanti koptu, yeniden baglaniyor...");
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(1000));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        applySettings();
        Serial.println("[WiFi] Yeniden baglandi: " + WiFi.localIP().toString());
    }
    else
    {
        Serial.println("[WiFi] Yeniden baglanti basarisiz");
    }
}

bool wifiIsConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
