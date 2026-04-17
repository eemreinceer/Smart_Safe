#ifndef CONFIG_H
#define CONFIG_H

// ══════════════════════════════════════════════
//  SİMÜLASYON MODU
//  true  → Donanım yok, Serial ile test
//  false → Gerçek sensörler bağlı
// ══════════════════════════════════════════════
#define SIMULATION_MODE true

// ══════════════════════════════════════════════
//  WiFi Ayarları
// ══════════════════════════════════════════════
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

// ══════════════════════════════════════════════
//  Firebase Ayarları
// ══════════════════════════════════════════════
#define API_KEY         ""
#define DATABASE_URL    ""
#define AUTO_INIT_FIREBASE true

// ─── YENİ: Firebase Auth (Dashboard admin girişi için) ───
#define FIREBASE_AUTH_EMAIL    ""
#define FIREBASE_AUTH_PASSWORD ""

// ══════════════════════════════════════════════
//  NTP Ayarları (YENİ - Gerçek Zaman Damgası)
// ══════════════════════════════════════════════
#define NTP_SERVER      "pool.ntp.org"
#define NTP_GMT_OFFSET  10800       // Türkiye UTC+3 → 3*3600
#define NTP_DAYLIGHT    0

// ══════════════════════════════════════════════
//  Cihaz Bilgileri (YENİ - Dashboard için)
// ══════════════════════════════════════════════
#define DEVICE_ID       ""
#define FIRMWARE_VER    ""

// ══════════════════════════════════════════════
//  Sistem Ayarları
// ══════════════════════════════════════════════
#define LOCKDOWN_DURATION_MS  30000
#define MAX_FAIL_ATTEMPTS     3
#define LOCK_OPEN_DURATION_MS 3000

// ══════════════════════════════════════════════
//  Pin Tanımları (GERÇEK DONANIM İÇİN)
//  ESP32-CAM uyumlu pinler!
// ══════════════════════════════════════════════
#if !SIMULATION_MODE

    #define RELAY_PIN    2
    #define FP_RX        13
    #define FP_TX        15
    #define DF_RX        14
    #define DF_TX        12

#endif

#endif