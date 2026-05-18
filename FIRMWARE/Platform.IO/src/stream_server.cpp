#include "stream_server.h"
#include "esp_camera.h"
#include "esp_http_server.h"
#include "Arduino.h"
#include "camera.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t* fb        = NULL;
    esp_err_t    res       = ESP_OK;
    size_t       jpgLen    = 0;
    uint8_t*     jpgBuf    = NULL;
    char         partBuf[64];   // char dizisi — önceki kod yanlışlıkla char* dizisi kullanıyordu

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    while (true)
    {
        // cameraMutex için sınırlı bekleme — capturePhotoBase64'ü bloklamaz
        if (xSemaphoreTake(cameraMutex, pdMS_TO_TICKS(200)))
        {
            fb = esp_camera_fb_get();
            xSemaphoreGive(cameraMutex);
        }

        if (!fb)
        {
            res = ESP_FAIL;
        }
        else if (fb->format != PIXFORMAT_JPEG)
        {
            bool ok = frame2jpg(fb, 80, &jpgBuf, &jpgLen);
            esp_camera_fb_return(fb);
            fb = NULL;
            if (!ok) res = ESP_FAIL;
        }
        else
        {
            jpgLen = fb->len;
            jpgBuf = fb->buf;
        }

        if (res == ESP_OK)
        {
            size_t hlen = snprintf(partBuf, sizeof(partBuf), _STREAM_PART, jpgLen);
            res = httpd_resp_send_chunk(req, partBuf, hlen);
        }
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, (const char*)jpgBuf, jpgLen);
        if (res == ESP_OK)
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));

        if (fb)         { esp_camera_fb_return(fb); fb = NULL; jpgBuf = NULL; }
        else if (jpgBuf){ free(jpgBuf); jpgBuf = NULL; }

        if (res != ESP_OK) break;

        vTaskDelay(pdMS_TO_TICKS(33));  // ~30 FPS üst sınır
    }
    return res;
}

void startStreamServer()
{
    httpd_config_t config  = HTTPD_DEFAULT_CONFIG();
    config.server_port     = 81;
    config.ctrl_port       = 32769;

    httpd_uri_t stream_uri = {
        .uri      = "/stream",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("[STREAM] MJPEG sunucu baslatildi (port 81)");
    }
}
