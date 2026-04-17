#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "fingerprint.h"
#include <Adafruit_Fingerprint.h>

HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger(&fingerSerial);

void initFingerprint()
{
    fingerSerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
    finger.begin(57600);

    if (finger.verifyPassword())
        Serial.println("[FP] ✅ Parmak izi sensörü hazır");
    else
        Serial.println("[FP] ❌ Parmak izi sensörü hatası");
}

int readFingerprint()
{
    if (finger.getImage() != FINGERPRINT_OK)
        return -1;

    if (finger.image2Tz() != FINGERPRINT_OK)
        return -1;

    if (finger.fingerSearch() != FINGERPRINT_OK)
        return -2;

    return finger.fingerID;
}

#endif