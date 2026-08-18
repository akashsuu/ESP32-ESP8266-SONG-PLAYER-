#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// 1. WI-FI CONFIGURATION
// ============================================================================
// Replace with your Laptop / Router Wi-Fi details
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASS           "YOUR_WIFI_PASSWORD"

// Access Point Fallback (If home Wi-Fi is unreachable)
#define AP_SSID             "ESP32-CCTV-CAM"
#define AP_PASS             "12345678"

// ============================================================================
// 2. OV7670 CAMERA PIN MAP (ESP32-WROOM-DA)
// ============================================================================
#define PWDN_GPIO_NUM       -1      // Connect to GND
#define RESET_GPIO_NUM      -1      // Connect to 3.3V
#define XCLK_GPIO_NUM       32      // xlk
#define SIOD_GPIO_NUM       33      // sda (Requires 4.7k pull-up to 3.3V)
#define SIOC_GPIO_NUM       13      // scl (Requires 4.7k pull-up to 3.3V)

// Data Bus D7..D0
#define Y9_GPIO_NUM         35      // d7
#define Y8_GPIO_NUM         34      // d6
#define Y7_GPIO_NUM         39      // d5 (svn)
#define Y6_GPIO_NUM         36      // d4 (svp)
#define Y5_GPIO_NUM         19      // d3
#define Y4_GPIO_NUM         21      // d2
#define Y3_GPIO_NUM         22      // d1
#define Y2_GPIO_NUM         25      // d0

// Synchronization Clocks
#define VSYNC_GPIO_NUM      27      // vs
#define HREF_GPIO_NUM       14      // hs
#define PCLK_GPIO_NUM       26      // plk

#define XCLK_FREQ_HZ        10000000

#endif // CONFIG_H
