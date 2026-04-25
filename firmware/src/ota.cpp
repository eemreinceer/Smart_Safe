#include <ArduinoOTA.h>

void initOTA()
{
    ArduinoOTA.setHostname("smart-safe");

    ArduinoOTA.onStart([]() {
        Serial.println("OTA Start");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("OTA End");
    });

    ArduinoOTA.begin();

    Serial.println("OTA ready");
}

void handleOTA()
{
    ArduinoOTA.handle();
}