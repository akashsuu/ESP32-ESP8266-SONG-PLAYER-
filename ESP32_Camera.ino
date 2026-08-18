#include <Arduino.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "config.h"
#include "camera_driver.h"
#include "cctv_server.h"

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    delay(500);
    Serial.println("\n==========================================");
    Serial.println("  ESP32 + OV7670 Wi-Fi CCTV Live Camera   ");
    Serial.println("==========================================");

    // Initialize OV7670 Camera
    if (!initCamera()) {
        Serial.println("CRITICAL ERROR: OV7670 Camera Init Failed!");
    } else {
        Serial.println("OV7670 Camera Initialized Successfully.");
    }

    // Connect to Wi-Fi
    Serial.printf("Connecting to Wi-Fi: %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nSUCCESS: Wi-Fi Connected!");
        Serial.print("CCTV Camera IP Address: http://");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWi-Fi connection failed. Starting Access Point Fallback...");
        WiFi.softAP(AP_SSID, AP_PASS);
        Serial.print("CCTV Hotspot IP Address: http://");
        Serial.println(WiFi.softAPIP());
        Serial.printf("Connect Laptop Wi-Fi to '%s' (Password: '%s')\n", AP_SSID, AP_PASS);
    }

    // Start Streaming Web Server
    startCameraServer();
    Serial.println("\n>>> Open your Laptop browser and visit the IP address above! <<<\n");
}

void loop() {
    delay(1000); // Server runs asynchronously in background task
}
