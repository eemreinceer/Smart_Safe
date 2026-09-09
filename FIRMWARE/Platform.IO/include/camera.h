#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t cameraMutex;

void   initCamera();
String capturePhotoBase64();  // JPEG çek, base64 String döndür; hata varsa ""

#endif
