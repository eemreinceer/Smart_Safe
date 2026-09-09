// ══════════════════════════════════════════════
//  MOCK RFID — Serial üzerinden simülasyon
//
//  Kullanım (Serial Monitor'a yaz):
//    RFID:A1B2C3D4   → config.h'daki UID ile eşleşirse yetkili
//    RFID:XXXXXXXX   → eşleşmezse yetkisiz
// ══════════════════════════════════════════════

#include "config.h"

#if SIMULATION_MODE || RFID_MOCK

#include <Arduino.h>
#include "rfid.h"

void initRFID()
{
    Serial.println("[MOCK-RFID] RFID simülasyonu hazır");
    Serial.println("[MOCK-RFID] Komut: RFID:AABBCCDD");
}

int readRFID()
{
    if (!Serial.available())
        return -1;

    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (!input.startsWith("RFID:"))
        return -1;

    String uid = input.substring(5);
    Serial.printf("[MOCK-RFID] Kart okundu → UID: %s\n", uid.c_str());

    // Yetkili UID listesiyle karşılaştır
    if (uid.length() != 8 && uid.length() != 14 && uid.length() != 20)
    {
        Serial.println("[MOCK-RFID] Gecersiz UID uzunlugu");
        return -2;
    }

    for (int i = 0; i < AUTHORIZED_UID_COUNT; i++)
    {
        if (String(AUTHORIZED_UIDS[i]).length() > 0 && uid == String(AUTHORIZED_UIDS[i]))
        {
            Serial.printf("[MOCK-RFID] ✅ Yetkili kart: %s\n", uid.c_str());
            return i + 1;   // 1'den başlayan kart index'i
        }
    }

    Serial.printf("[MOCK-RFID] ❌ Yetkisiz kart: %s\n", uid.c_str());
    return -2;
}

#endif
