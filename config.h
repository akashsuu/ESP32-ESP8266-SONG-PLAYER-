#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// 1. BLUETOOTH CONFIGURATION
// ============================================================================
#define BT_DEVICE_NAME      "ESP32-CAMERA"

// ============================================================================
// 2. SHUTTER BUTTON CONFIGURATION
// ============================================================================
// GPIO 15 has internal pull-up enabled by default; button connects GPIO 15 to GND
#define BUTTON_PIN          15
#define DEBOUNCE_DELAY_MS   50

// ============================================================================
// 3. 1.8" SPI TFT 128x160 (ST7735 V1.1) PIN MAP (Native ESP32 Hardware VSPI)
// ============================================================================
#define TFT_CS              5       // p5  (Native VSPI CS)
#define TFT_DC              16      // p16 (Data / Command Select)
#define TFT_RST             17      // p17 (Hardware Reset)
#define TFT_MOSI            23      // p23 (Native VSPI MOSI)
#define TFT_SCLK            18      // p18 (Native VSPI SCLK)

// Tab Type Options: INITR_BLACKTAB, INITR_REDTAB, INITR_GREENTAB, INITR_144GREENTAB
#define ST7735_TAB_TYPE     INITR_BLACKTAB

// TFT Display Physical Dimensions
#define TFT_WIDTH           128
#define TFT_HEIGHT          160

// ============================================================================
// 4. OV7670 CAMERA PIN MAP (ESP32-WROOM-DA without PSRAM)
// ============================================================================
// Camera Clock and I2C (SCCB)
#define PWDN_GPIO_NUM       -1      // Power down (connect pin to GND)
#define RESET_GPIO_NUM      -1      // Reset (connect pin to 3.3V)
#define XCLK_GPIO_NUM       32      // System Clock generator
#define SIOD_GPIO_NUM       33      // SCCB Data (SDA) - Requires 4.7k pull-up
#define SIOC_GPIO_NUM       13      // SCCB Clock (SCL) - Requires 4.7k pull-up

// Camera Data Bus (D7..D0)
#define Y9_GPIO_NUM         35      // D7 (Input-only GPIO)
#define Y8_GPIO_NUM         34      // D6 (Input-only GPIO)
#define Y7_GPIO_NUM         39      // D5 (Input-only GPIO - VN)
#define Y6_GPIO_NUM         36      // D4 (Input-only GPIO - VP)
#define Y5_GPIO_NUM         19      // D3
#define Y4_GPIO_NUM         21      // D2
#define Y3_GPIO_NUM         22      // D1
#define Y2_GPIO_NUM         25      // D0

// Synchronization Clocks
#define VSYNC_GPIO_NUM      27      // Vertical Sync
#define HREF_GPIO_NUM       14      // Horizontal Reference
#define PCLK_GPIO_NUM       26      // Pixel Clock

// Camera Clock Frequency (10MHz - 20MHz for OV7670)
#define XCLK_FREQ_HZ        10000000

// ============================================================================
// 5. PROTOCOL & TRANSFER SETTINGS
// ============================================================================
#define BT_PAYLOAD_CHUNK_SZ 512     // Size of binary payload per Bluetooth packet

#endif // CONFIG_H
