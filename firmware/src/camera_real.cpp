#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "camera.h"
#include "firebase.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

SemaphoreHandle_t cameraMutex;

void initCamera()
{
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0    = 5;
    config.pin_d1    = 18;
    config.pin_d2    = 19;
    config.pin_d3    = 21;
    config.pin_d4    = 36;
    config.pin_d5    = 39;
    config.pin_d6    = 34;
    config.pin_d7    = 35;
    config.pin_xclk  = 0;
    config.pin_pclk  = 22;
    config.pin_vsync = 25;
    config.pin_href  = 23;

    config.pin_sccb_sda = 26;
    config.pin_sccb_scl = 27;

    config.pin_pwdn  = 32;
    config.pin_reset = -1;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size   = FRAMESIZE_VGA;  // 640x480 çözünürlük
    config.jpeg_quality = 10;             // Daha yüksek kalite (düşük rakam = yüksek kalite)
    config.fb_count     = 2;

    if (esp_camera_init(&config) != ESP_OK)
    {
        Serial.println("[CAM] ❌ Kamera başlatılamadı");
        return;
    }

    cameraMutex = xSemaphoreCreateMutex();

    Serial.println("[CAM] ✅ Kamera hazır");
}

void capturePhoto()
{
    if (xSemaphoreTake(cameraMutex, portMAX_DELAY))
    {
        camera_fb_t *fb = esp_camera_fb_get();

        if (!fb)
        {
            Serial.println("[CAM] ❌ Fotoğraf çekilemedi");
            xSemaphoreGive(cameraMutex);
            return;
        }

        Serial.printf("[CAM] 📸 Fotoğraf çekildi (%d bytes)\n", fb->len);

        // Firebase Storage'a yükle
        uploadPhoto(fb->buf, fb->len);

        esp_camera_fb_return(fb);
        xSemaphoreGive(cameraMutex);
    }
}

#endif