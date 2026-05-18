#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "camera.h"

SemaphoreHandle_t cameraMutex = NULL;

void initCamera()
{
    Serial.println("[MOCK-CAM] ✅ Kamera simülasyonu hazır");
}

String capturePhotoBase64()
{
    Serial.println("[MOCK-CAM] 📸 Fotoğraf çekildi (simülasyon — fotoğraf yok)");
    return "";
}

#endif