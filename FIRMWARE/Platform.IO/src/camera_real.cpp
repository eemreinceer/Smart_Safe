#include "config.h"

#if !SIMULATION_MODE

#include <Arduino.h>
#include "camera.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/base64.h"

SemaphoreHandle_t cameraMutex;
static bool       cameraReady  = false;
static int        camFailCount = 0;

static bool cameraInit()
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

    if (psramFound())
    {
        config.frame_size   = FRAMESIZE_QVGA;  // 320x240 — yüz tespiti + Firebase uyumlu boyut
        config.jpeg_quality = 8;               // ~28KB JPEG → ~38KB base64, SSL heap güvenli
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
    }
    else
    {
        config.frame_size   = FRAMESIZE_QQVGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_DRAM;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("[CAM] ❌ Kamera başlatılamadı (0x%x)\n", err);
        return false;
    }

    Serial.println("[CAM] ✅ Kamera hazır");
    return true;
}

void initCamera()
{
    if (cameraMutex == NULL)
        cameraMutex = xSemaphoreCreateMutex();

    cameraReady = cameraInit();
}

String capturePhotoBase64()
{
    if (!cameraReady)
    {
        Serial.println("[CAM] Kamera hazır değil, atlanıyor");
        return "";
    }

    // Heap yetersizse çekme — malloc + String birlikte ~80KB ister
    if (ESP.getFreeHeap() < 90000)
    {
        Serial.printf("[CAM] Heap yetersiz (%u), fotoğraf atlandı\n", ESP.getFreeHeap());
        return "";
    }

    if (xSemaphoreTake(cameraMutex, pdMS_TO_TICKS(5000)) != pdTRUE)
        return "";

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("[CAM] ❌ Frame alınamadı");
        xSemaphoreGive(cameraMutex);

        camFailCount++;
        Serial.printf("[CAM] Ardışık hata: %d\n", camFailCount);

        // 3 ardışık hata → kamerayı yeniden başlat
        if (camFailCount >= 3)
        {
            Serial.println("[CAM] Kamera yeniden başlatılıyor...");
            cameraReady  = false;   // önce false — araya başka çağrı girmesin
            esp_camera_deinit();
            delay(500);
            cameraReady  = cameraInit();
            camFailCount = 0;
        }
        return "";
    }

    camFailCount = 0;  // başarılı frame → sayacı sıfırla
    Serial.printf("[CAM] 📸 Fotoğraf çekildi (%d bytes)\n", fb->len);

    size_t b64Len   = 0;
    size_t b64BufSz = ((fb->len + 2) / 3) * 4 + 32;

    uint8_t* b64Buf = (uint8_t*)malloc(b64BufSz);
    if (!b64Buf)
    {
        Serial.println("[CAM] malloc basarisiz");
        esp_camera_fb_return(fb);
        xSemaphoreGive(cameraMutex);
        return "";
    }

    const char* prefix    = "data:image/jpeg;base64,";
    size_t      prefixLen = strlen(prefix);
    memcpy(b64Buf, prefix, prefixLen);
    mbedtls_base64_encode(b64Buf + prefixLen, b64BufSz - prefixLen,
                          &b64Len, fb->buf, fb->len);
    b64Buf[prefixLen + b64Len] = '\0';

    Serial.printf("[CAM] Base64 hazir (%u bytes)\n", prefixLen + b64Len);

    String result = String((char*)b64Buf);
    free(b64Buf);
    esp_camera_fb_return(fb);
    xSemaphoreGive(cameraMutex);
    return result;
}

#endif