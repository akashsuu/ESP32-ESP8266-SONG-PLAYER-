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
// Camera Clock and I2C (SCCB)
#define PWDN_GPIO_NUM       -1      // Connect to GND
#define RESET_GPIO_NUM      -1      // Connect to 3.3V
#define XCLK_GPIO_NUM       32      // xlk (p32)
#define SIOD_GPIO_NUM       33      // sda (p33)
#define SIOC_GPIO_NUM       13      // scl (p13)

// Data Bus D7..D0
#define Y9_GPIO_NUM         35      // d7 (p35)
#define Y8_GPIO_NUM         34      // d6 (p34)
#define Y7_GPIO_NUM         39      // d5 (svn)
#define Y6_GPIO_NUM         36      // d4 (svp)
#define Y5_GPIO_NUM         19      // d3 (p19)
#define Y4_GPIO_NUM         21      // d2 (p21)
#define Y3_GPIO_NUM         22      // d1 (p22)
#define Y2_GPIO_NUM         25      // d0 (p25)

// Synchronization Clocks
#define VSYNC_GPIO_NUM      27      // vs
#define HREF_GPIO_NUM       14      // hs
#define PCLK_GPIO_NUM       26      // plk

#define XCLK_FREQ_HZ        10000000

#endif // CONFIG_H
