#include "display_driver.h"

// Instantiate Adafruit ST7735 TFT driver using hardware SPI
static Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

void initDisplay() {
    // Initialize 1.8" TFT with ST7735R red tab / black tab layout
    tft.initR(INITR_BLACKTAB);
    // Set landscape rotation (160 wide x 128 high)
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
}

void showWaitingScreen() {
    tft.fillScreen(ST77XX_BLACK);
    
    // Draw outer cyan box border
    tft.drawRect(4, 4, 152, 120, ST77XX_CYAN);
    tft.drawRect(6, 6, 148, 116, ST77XX_CYAN);

    // Title Header
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(2);
    tft.setCursor(24, 16);
    tft.print("ESP32 CAM");

    // Status Label
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(44, 46);
    tft.print("BLUETOOTH");

    tft.setTextColor(ST77XX_MAGENTA);
    tft.setTextSize(1);
    tft.setCursor(44, 60);
    tft.print("WAITING...");

    // Advertised Device Name
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.setCursor(32, 90);
    tft.print("ESP32-CAMERA");
}

void showConnectedScreen() {
    tft.fillScreen(ST77XX_BLUE);
    tft.drawRect(4, 4, 152, 120, ST77XX_WHITE);

    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(24, 40);
    tft.print("BLUETOOTH");

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(24, 68);
    tft.print("CONNECTED");
}

void renderCameraFrame(camera_fb_t* fb) {
    if (!fb || !fb->buf) return;

    // QQVGA frame is 160 pixels wide x 120 pixels high (RGB565 = 38400 bytes)
    // ST7735 in landscape mode 1 (160x128)
    // Render centered vertically at y=4
    tft.startWrite();
    tft.setAddrWindow(0, 4, 160, 120);

    // Fast SPI push of camera pixels to TFT screen
    uint16_t* pixels = (uint16_t*)fb->buf;
    size_t pixel_count = fb->width * fb->height;

    tft.writePixels(pixels, pixel_count);
    tft.endWrite();
}
