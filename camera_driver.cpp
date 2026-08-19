#include "camera_driver.h"
#include "driver/ledc.h"
#include <Wire.h>

static bool testOV7670_SCCB(int sda_pin, int scl_pin) {
    Serial.printf("\n--- TESTING 2017 OV7670 SCCB REGISTER READ (SDA=GPIO%d, SCL=GPIO%d) ---\n", sda_pin, scl_pin);
    pinMode(sda_pin, INPUT_PULLUP);
    pinMode(scl_pin, INPUT_PULLUP);

    Wire.begin(sda_pin, scl_pin, 50000); // 50 kHz SCCB speed
    delay(50);

    // Read PID Register 0x0A
    Wire.beginTransmission(0x21);
    Wire.write(0x0A);
    uint8_t err1 = Wire.endTransmission(true);

    uint8_t pid = 0;
    if (err1 == 0) {
        delay(5);
        Wire.requestFrom(0x21, 1);
        if (Wire.available()) pid = Wire.read();
    }

    // Read VER Register 0x0B
    Wire.beginTransmission(0x21);
    Wire.write(0x0B);
    uint8_t err2 = Wire.endTransmission(true);

    uint8_t ver = 0;
    if (err2 == 0) {
        delay(5);
        Wire.requestFrom(0x21, 1);
        if (Wire.available()) ver = Wire.read();
    }

    Serial.printf("  [★ SENSOR INFO ★] OV7670 PID (0x0A) = 0x%02X | VER (0x0B) = 0x%02X\n", pid, ver);
    Wire.end();
    return (pid == 0x76 || pid == 0x70 || ver == 0x70 || ver == 0x73);
}

bool initCamera() {
    // 1. Enable ESP32 internal pull-up resistors on SCCB I2C pins (SDA=33, SCL=13)
    pinMode(SIOD_GPIO_NUM, INPUT_PULLUP);
    pinMode(SIOC_GPIO_NUM, INPUT_PULLUP);

    // 2. Pre-start XCLK clock on GPIO 32 at 10MHz
    ledc_timer_config_t timer_conf;
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_1_BIT;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.freq_hz = 10000000; // 10 MHz
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf;
    channel_conf.gpio_num = (gpio_num_t)XCLK_GPIO_NUM; // GPIO 32
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = LEDC_CHANNEL_0;
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = LEDC_TIMER_0;
    channel_conf.duty = 1;
    channel_conf.hpoint = 0;
    ledc_channel_config(&channel_conf);

    delay(200); // Clock warmup

    // 3. Diagnostic Direct SCCB Register 0x0A Test
    testOV7670_SCCB(SIOD_GPIO_NUM, SIOC_GPIO_NUM);

    // 4. Configure esp_camera driver
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;

    config.xclk_freq_hz = 10000000; // 10 MHz
    config.pixel_format = PIXFORMAT_RGB565; // OV7670 native format
    config.frame_size   = FRAMESIZE_QQVGA;  // 160x120 pixels
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.grab_mode    = CAMERA_GRAB_LATEST;
    config.fb_location  = CAMERA_FB_IN_DRAM;
    config.sccb_i2c_port = -1; // Use Software Bit-Banged SCCB (Required for non-FIFO OV7670)

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("OV7670 init at 10MHz returned 0x%x. Retrying at 12MHz...\n", err);
        
        config.xclk_freq_hz = 12000000;
        err = esp_camera_init(&config);

        if (err != ESP_OK) {
            Serial.printf("OV7670 init retry failed: 0x%x\n", err);
            return false;
        }
    }

    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_hmirror(s, 1);
        s->set_vflip(s, 1);
    }

    return true;
}

camera_fb_t* captureFrame() {
    return esp_camera_fb_get();
}

void releaseFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}
