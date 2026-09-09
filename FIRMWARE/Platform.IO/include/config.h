#ifndef CONFIG_H
#define CONFIG_H

// ══════════════════════════════════════════════
//  SİMÜLASYON MODU
//  true  → Donanım yok, Serial ile test
//  false → Gerçek sensörler bağlı
// ══════════════════════════════════════════════
#ifndef SIMULATION_MODE
#define SIMULATION_MODE 1
#endif
#ifndef RFID_MOCK
#define RFID_MOCK SIMULATION_MODE
#endif

// ══════════════════════════════════════════════
//  WiFi Ayarları
// ══════════════════════════════════════════════
// Wokwi simulator için Wokwi-GUEST kullanın (şifresiz, internet açık).
// Gerçek donanımda kendi WiFi'nızı yazın.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID                 "Wokwi-GUEST"
#define WIFI_PASSWORD             ""
#define FIREBASE_API_KEY          ""
#define FIREBASE_DATABASE_URL     ""
#define FIREBASE_AUTH_EMAIL       ""
#define FIREBASE_AUTH_PASSWORD    ""
#define FIREBASE_ROOT_CA_PEM      ""
#define OTA_PASSWORD_HASH         ""
#if SIMULATION_MODE
#define AUTHORIZED_UID_1          "A1B2C3D4"
#else
#define AUTHORIZED_UID_1          ""
#endif
#define AUTHORIZED_UID_2          ""
#define AUTHORIZED_UID_3          ""
#endif

// ══════════════════════════════════════════════
//  Firebase Ayarları
// ══════════════════════════════════════════════
#define API_KEY            FIREBASE_API_KEY
#define DATABASE_URL       FIREBASE_DATABASE_URL
#define AUTO_INIT_FIREBASE 1

// ══════════════════════════════════════════════
//  NTP Ayarları
// ══════════════════════════════════════════════
#define NTP_SERVER      "pool.ntp.org"
#define NTP_GMT_OFFSET  10800
#define NTP_DAYLIGHT    0

// ══════════════════════════════════════════════
//  Cihaz Bilgileri
// ══════════════════════════════════════════════
#define DEVICE_ID       "safe_001"
#define FIRMWARE_VER    "1.0.0"

// ══════════════════════════════════════════════
//  Sistem Ayarları
// ══════════════════════════════════════════════
#define LOCKDOWN_DURATION_MS  30000
#define MAX_FAIL_ATTEMPTS     3
#define LOCK_OPEN_DURATION_MS 3000

// ══════════════════════════════════════════════
//  Yetkili RFID Kart UID'leri
//  Format: 4 byte hex, büyük harf, örn: "A1B2C3D4"
//  Kartını okumak için SIMULATION_MODE=false yap,
//  Serial Monitor'da UID otomatik yazdırılır.
// ══════════════════════════════════════════════
#define AUTHORIZED_UID_COUNT 3
static const char* AUTHORIZED_UIDS[AUTHORIZED_UID_COUNT] = {
    AUTHORIZED_UID_1,
    AUTHORIZED_UID_2,
    AUTHORIZED_UID_3
};

// ══════════════════════════════════════════════
//  Pin Tanımları (GERÇEK DONANIM)
//  ESP32-CAM AI Thinker uyumlu
// ══════════════════════════════════════════════
#if !SIMULATION_MODE

    // Solenoid kilit (IRLZ44N MOSFET Gate)
    #define RELAY_PIN        2

    // RC522 RFID — HSPI (özel pin mapping)
    #define RFID_SS_PIN     15   // SDA/SS
    #define RFID_SCK_PIN    14   // SCK
    #define RFID_MISO_PIN    4   // MISO
    #define RFID_MOSI_PIN   13   // MOSI
    // RST → 3.3V'a sabit bağlı (UNUSED_PIN)

    // DFPlayer Mini — UART (tek yönlü, sadece TX)
    #define DF_TX           12   // ESP32 TX → DFPlayer RX
    // DF_RX bağlı değil, ACK devre dışı

#endif

#endif
