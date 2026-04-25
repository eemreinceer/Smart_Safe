#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "rfid.h"

// RST pini donanımda 3.3V'a sabit bağlı
MFRC522 rfid(RFID_SS_PIN, MFRC522::UNUSED_PIN);

// ── UID byte dizisini hex String'e çevir ─────
static String uidToString(MFRC522::Uid &uid)
{
    String result = "";
    for (byte i = 0; i < uid.size; i++)
    {
        if (uid.uidByte[i] < 0x10)
            result += "0";
        result += String(uid.uidByte[i], HEX);
    }
    result.toUpperCase();
    return result;
}

// ── Başlatma ─────────────────────────────────
void initRFID()
{
    // ESP32-CAM'de HSPI pinlerini özel olarak tanımla
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);

    rfid.PCD_Init();
    rfid.PCD_DumpVersionToSerial();   // Modül versiyonunu yazdır

    Serial.println("[RFID] ✅ RC522 hazır");
    Serial.println("[RFID] Yetkili UID listesi:");
    for (int i = 0; i < AUTHORIZED_UID_COUNT; i++)
        Serial.printf("  [%d] %s\n", i + 1, AUTHORIZED_UIDS[i]);
}

// ── Kart Okuma ───────────────────────────────
int readRFID()
{
    // Kart var mı?
    if (!rfid.PICC_IsNewCardPresent())
        return -1;

    // UID okunabildi mi?
    if (!rfid.PICC_ReadCardSerial())
        return -1;

    String uid = uidToString(rfid.uid);
    Serial.printf("[RFID] 📡 Kart okundu → UID: %s\n", uid.c_str());

    // Yetkili UID listesiyle karşılaştır
    for (int i = 0; i < AUTHORIZED_UID_COUNT; i++)
    {
        if (uid == String(AUTHORIZED_UIDS[i]))
        {
            Serial.printf("[RFID] ✅ Yetkili kart: %s\n", uid.c_str());
            rfid.PICC_HaltA();        // Kartı durdur
            rfid.PCD_StopCrypto1();
            return i + 1;
        }
    }

    Serial.printf("[RFID] ❌ Yetkisiz kart: %s\n", uid.c_str());
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return -2;
}

#endif