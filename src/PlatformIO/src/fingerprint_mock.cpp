// ══════════════════════════════════════════════
//  MOCK FINGERPRINT - Serial üzerinden simülasyon
//
//  Kullanım (Serial Monitor'a yaz):
//    FINGER:1    → Yetkili kullanıcı ID=1
//    FINGER:2    → Yetkili kullanıcı ID=2
//    FINGER:99   → Yetkisiz kullanıcı
//    FINGER:0    → Tanımsız parmak izi
// ══════════════════════════════════════════════

#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "fingerprint.h"

// ── Yetkili kullanıcı listesi ────────────────
static const int authorizedIDs[] = {1, 2, 3, 4, 5};
static const int authorizedCount = 5;

// ── Başlatma ─────────────────────────────────
void initFingerprint()
{
    Serial.println("[MOCK-FP] Parmak izi simülasyonu hazır");
    Serial.println("[MOCK-FP] Komutlar: FINGER:1 (yetkili) | FINGER:99 (yetkisiz)");
}

// ── Okuma ────────────────────────────────────
int readFingerprint()
{
    if (!Serial.available())
        return -1;   // hiç komut yok, boşta

    String input = Serial.readStringUntil('\n');
    input.trim();

    // "FINGER:X" formatını kontrol et
    if (!input.startsWith("FINGER:"))
        return -1;   // bu komut bize ait değil

    // ID'yi çıkar
    int id = input.substring(7).toInt();

    Serial.printf("[MOCK-FP] Parmak izi okundu → ID: %d\n", id);

    // Yetkili mi kontrol et
    for (int i = 0; i < authorizedCount; i++)
    {
        if (authorizedIDs[i] == id)
        {
            Serial.printf("[MOCK-FP] ✅ Yetkili kullanıcı: %d\n", id);
            return id;   // yetkili → pozitif ID döner
        }
    }

    Serial.printf("[MOCK-FP] ❌ Yetkisiz parmak izi: %d\n", id);
    return -2;   // yetkisiz → -2 döner
}

#endif