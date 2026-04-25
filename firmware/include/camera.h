#ifndef CAMERA_H
#define CAMERA_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t cameraMutex;

void initCamera();
void capturePhoto();

#endif