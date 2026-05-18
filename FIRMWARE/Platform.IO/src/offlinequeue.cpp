#include <Arduino.h>
#include <Preferences.h>
#include "offlinequeue.h"
#include "firebase.h"

Preferences offlinePrefs;

static int readIndex  = 0;
static int writeIndex = 0;

#define MAX_OFFLINE_LOGS 50  // NVS namespace sınırı ~128 entry; 50 güvenli üst sınır

void initOfflineQueue()
{
    offlinePrefs.begin("offlineLogs", false);
    readIndex  = offlinePrefs.getInt("read",  0);
    writeIndex = offlinePrefs.getInt("write", 0);
    Serial.printf("[OFFLINE] Queue hazir (bekleyen: %d)\n", writeIndex - readIndex);
}

void storeOfflineLog(String status, String id)
{
    // Kuyruk doluysa en eski kaydı sil (dairesel)
    if ((writeIndex - readIndex) >= MAX_OFFLINE_LOGS)
    {
        offlinePrefs.remove(("log" + String(readIndex)).c_str());
        readIndex++;
        offlinePrefs.putInt("read", readIndex);
        Serial.println("[OFFLINE] Kuyruk dolu, en eski log silindi");
    }

    String key     = "log" + String(writeIndex);
    String payload = status + "," + id + "," + String(millis());

    offlinePrefs.putString(key.c_str(), payload);
    writeIndex++;
    offlinePrefs.putInt("write", writeIndex);

    Serial.println("[OFFLINE] Log kaydedildi: " + payload);
}

void trySyncOfflineLogs()
{
    if (readIndex >= writeIndex)
        return;

    Serial.printf("[OFFLINE] Sync basliyor (%d log bekliyor)\n", writeIndex - readIndex);

    int synced = 0;

    while (readIndex < writeIndex && synced < 3)  // en fazla 3 log per çağrı
    {
        // Her iterasyonda heap kontrolü
        if (ESP.getFreeHeap() < 80000)
        {
            Serial.printf("[OFFLINE] Heap yetersiz (%u), sync ertelendi\n", ESP.getFreeHeap());
            break;
        }

        String key     = "log" + String(readIndex);
        String payload = offlinePrefs.getString(key.c_str(), "");

        if (payload.length() == 0)
        {
            // Bozuk/eksik kayıt, atla
            offlinePrefs.remove(key.c_str());
            readIndex++;
            offlinePrefs.putInt("read", readIndex);
            continue;
        }

        int p1 = payload.indexOf(",");
        int p2 = payload.lastIndexOf(",");

        if (p1 < 0 || p2 <= p1)
        {
            // Parse edilemeyen kayıt, atla
            offlinePrefs.remove(key.c_str());
            readIndex++;
            offlinePrefs.putInt("read", readIndex);
            continue;
        }

        String status = payload.substring(0, p1);
        String id     = payload.substring(p1 + 1, p2);

        if (sendLog(status, id))
        {
            offlinePrefs.remove(key.c_str());
            readIndex++;
            offlinePrefs.putInt("read", readIndex);
            synced++;
            Serial.printf("[OFFLINE] Log senkronize edildi (%d kaldi)\n", writeIndex - readIndex);

            // SSL heap'in toparlanması için kısa bekleme
            vTaskDelay(pdMS_TO_TICKS(600));
        }
        else
        {
            // Sunucuya ulaşılamadı — bu sefer bırak, sonraki çağrıda tekrar dene
            break;
        }
    }
}
