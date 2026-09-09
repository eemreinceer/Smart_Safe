#include <ArduinoOTA.h>
#include "config.h"

static bool otaReady = false;

void initOTA()
{
    if (strlen(OTA_PASSWORD_HASH) != 32)
    {
        Serial.println("[OTA] Gecerli password hash yok; OTA fail-closed devre disi.");
        return;
    }

    ArduinoOTA.setHostname("smart-safe");
    ArduinoOTA.setPasswordHash(OTA_PASSWORD_HASH);

    ArduinoOTA.onStart([]() {
        Serial.println("OTA Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("OTA End");
    });

    ArduinoOTA.begin();
    otaReady = true;

    Serial.println("OTA ready");
}

void handleOTA()
{
    if (otaReady)
        ArduinoOTA.handle();
}
