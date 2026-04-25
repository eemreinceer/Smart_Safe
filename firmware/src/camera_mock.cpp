#include "config.h"

#if SIMULATION_MODE

#include <Arduino.h>
#include "camera.h"
#include "firebase.h"

void initCamera()
{
    Serial.println("[MOCK-CAM] ✅ Kamera simülasyonu hazır");
}

void capturePhoto()
{
    Serial.println("[MOCK-CAM] 📸 Fotoğraf çekildi (simülasyon)");

    // Gerçek buffer yok, NULL ve 0 gönderiyoruz
    // firebase.cpp içinde simülasyon dalı çalışacak
    uploadPhoto(NULL, 0);
}

#endif