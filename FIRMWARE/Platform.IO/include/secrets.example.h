#pragma once

// Bu dosyayı secrets.h olarak kopyalayın. secrets.h Git tarafından izlenmez.
// Gerçek değerleri asla commit etmeyin.
#define WIFI_SSID                 "YOUR_WIFI_SSID"
#define WIFI_PASSWORD             "YOUR_WIFI_PASSWORD"
#define FIREBASE_API_KEY          "YOUR_FIREBASE_WEB_API_KEY"
#define FIREBASE_DATABASE_URL     "https://YOUR_PROJECT-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH_EMAIL       "device-safe-001@example.invalid"
#define FIREBASE_AUTH_PASSWORD    "DEVICE_ACCOUNT_PASSWORD"

// Google/Firebase endpoint'ini imzalayan CA'nın PEM sertifikası.
// Boş bırakılırsa Firebase güvenli biçimde devre dışı kalır.
#define FIREBASE_ROOT_CA_PEM      "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n"

// `echo -n 'strong-password' | md5sum` ile değil, ArduinoOTA'nın beklediği
// MD5 password hash'i provisioning sürecinde üretin. Boşsa OTA başlatılmaz.
#define OTA_PASSWORD_HASH         "YOUR_32_HEX_MD5_PASSWORD_HASH"

// RC522 UID'leri 4, 7 veya 10 byte olabilir: 8, 14 veya 20 hex karakter.
// Kullanılmayan slotları boş bırakın.
#define AUTHORIZED_UID_1          "A1B2C3D4"
#define AUTHORIZED_UID_2          ""
#define AUTHORIZED_UID_3          ""
