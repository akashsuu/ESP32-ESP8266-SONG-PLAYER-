#include "cctv_server.h"
#include "esp_camera.h"
#include "img_converters.h"

// HTTP Server Handle
static httpd_handle_t stream_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace; boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// HTML Dashboard Interface
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32 CCTV Live Camera</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #121212;
            color: #ffffff;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        h1 { color: #00e676; margin-bottom: 10px; }
        .status {
            background-color: #1e1e1e;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            margin-bottom: 20px;
            border: 1px solid #333;
        }
        .stream-container {
            position: relative;
            border: 3px solid #00e676;
            border-radius: 12px;
            overflow: hidden;
            box-shadow: 0 8px 24px rgba(0, 230, 118, 0.2);
            background-color: #000;
        }
        img {
            display: block;
            max-width: 100%;
            width: 640px;
            height: 480px;
            object-fit: contain;
        }
        .controls {
            margin-top: 20px;
            display: flex;
            gap: 12px;
        }
        button {
            background-color: #00e676;
            color: #000;
            border: none;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            border-radius: 6px;
            cursor: pointer;
            transition: 0.2s;
        }
        button:hover { background-color: #00b0ff; color: #fff; }
    </style>
</head>
<body>
    <h1>🎥 ESP32 CCTV LIVE STREAM</h1>
    <div class="status">● LIVE STREAMING | OV7670 Camera</div>
    <div class="stream-container">
        <img src="/stream" id="cctv-stream" alt="Live CCTV Feed Loading...">
    </div>
    <div class="controls">
        <button onclick="document.getElementById('cctv-stream').src='/stream?'+Math.random()">Refresh Stream</button>
        <button onclick="takeSnapshot()">Take Snapshot</button>
    </div>
    <script>
        function takeSnapshot() {
            const img = document.getElementById('cctv-stream');
            const a = document.createElement('a');
            a.href = img.src;
            a.download = 'cctv_snapshot_' + Date.now() + '.jpg';
            a.click();
        }
    </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
}

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    Serial.println("Laptop client connected to CCTV MJPEG Stream!");

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Frame capture failed!");
            res = ESP_FAIL;
        } else {
            bool jpeg_converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);

            if (!jpeg_converted) {
                Serial.println("JPEG compression failed!");
                res = ESP_FAIL;
            }
        }

        if (res != ESP_OK) break;

        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK) break;
    }

    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        Serial.println("HTTP Server started on port 80");
    }
}
