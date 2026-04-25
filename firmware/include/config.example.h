#ifndef CONFIG_H
#define CONFIG_H

// ══════════════════════════════════════════════
//  SİMÜLASYON MODU
//  true  → Donanım yok, Serial ile test (Wokwi)
//  false → Gerçek sensörler bağlı
// ══════════════════════════════════════════════
#define SIMULATION_MODE true

// ══════════════════════════════════════════════
//  WiFi Ayarları
//  Wokwi: "Wokwi-GUEST" / ""  (internet açık, şifresiz)
//  Gerçek donanım: kendi WiFi bilgilerini gir
// ══════════════════════════════════════════════
#define WIFI_SSID       "Wokwi-GUEST"
#define WIFI_PASSWORD   ""

// ══════════════════════════════════════════════
//  Firebase Ayarları
//  Firebase Console > Project Settings > General > Web API Key
// ══════════════════════════════════════════════
#define API_KEY         "YOUR_FIREBASE_WEB_API_KEY"
#define DATABASE_URL    "https://YOUR_PROJECT_ID-default-rtdb.firebaseio.com"
#define AUTO_INIT_FIREBASE true

#define FIREBASE_AUTH_EMAIL    "your_admin@email.com"
#define FIREBASE_AUTH_PASSWORD "YOUR_FIREBASE_PASSWORD"

// ══════════════════════════════════════════════
//  NTP Ayarları
// ══════════════════════════════════════════════
#define NTP_SERVER      "pool.ntp.org"
#define NTP_GMT_OFFSET  10800   // UTC+3 (Türkiye)
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
    "YOUR_CARD_UID_1",   // 1. kart — buraya gerçek UID yaz
    "YOUR_CARD_UID_2",   // 2. kart
    "YOUR_CARD_UID_3"    // 3. kart
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
