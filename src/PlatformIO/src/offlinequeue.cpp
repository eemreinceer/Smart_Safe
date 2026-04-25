#include <Arduino.h>
#include <Preferences.h>
#include "offlinequeue.h"
#include "firebase.h"

Preferences offlinePrefs;

static int readIndex = 0;
static int writeIndex = 0;

void initOfflineQueue()
{
    offlinePrefs.begin("offlineLogs", false);

    readIndex = offlinePrefs.getInt("read", 0);
    writeIndex = offlinePrefs.getInt("write", 0);

    Serial.println("Offline queue ready");
}

void storeOfflineLog(String status, String id)
{
    String key = "log" + String(writeIndex);

    String payload = status + "," + id + "," + String(millis());

    offlinePrefs.putString(key.c_str(), payload);

    writeIndex++;

    offlinePrefs.putInt("write", writeIndex);

    Serial.println("Stored offline log: " + payload);
}

void trySyncOfflineLogs()
{
    while (readIndex < writeIndex)
    {
        String key = "log" + String(readIndex);

        String payload = offlinePrefs.getString(key.c_str(), "");

        if (payload == "")
        {
            readIndex++;
            offlinePrefs.putInt("read", readIndex);
            continue;
        }

        int p1 = payload.indexOf(",");
        int p2 = payload.lastIndexOf(",");

        String status = payload.substring(0, p1);
        String id = payload.substring(p1 + 1, p2);

        if (sendLog(status, id))
        {
            offlinePrefs.remove(key.c_str());

            readIndex++;

            offlinePrefs.putInt("read", readIndex);

            Serial.println("Offline log synced");
        }
        else
        {
            break;
        }
    }
}