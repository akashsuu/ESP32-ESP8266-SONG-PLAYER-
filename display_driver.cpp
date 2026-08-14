#include "display_driver.h"
#include <SPI.h>

// Use ESP32 Hardware SPI bus
static SPIClass tftSPI(HSPI);
static Adafruit_ST7735 tft = Adafruit_ST7735(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

void initDisplay() {
    // 1. Configure Pin Modes for DC, CS, RST
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_RST, OUTPUT);

    // 2. Perform Hardware Reset Pulse to wake up ST7735 controller
    digitalWrite(TFT_RST, HIGH);
    delay(10);
    digitalWrite(TFT_RST, LOW);
    delay(50);
    digitalWrite(TFT_RST, HIGH);
    delay(100);

    // 3. Initialize Hardware SPI bus (SCK=5, MOSI=17) at 27 MHz
    tftSPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tftSPI.setFrequency(27000000);

    // 4. Initialize ST7735 Display Driver
    tft.initR(ST7735_TAB_TYPE);

    // 5. Set Landscape Rotation (160 wide x 128 high)
    tft.setRotation(1);

    // 6. Diagnostic Startup Splash Test (RED -> GREEN -> BLUE -> BLACK)
    tft.fillScreen(ST77XX_RED);
    delay(150);
    tft.fillScreen(ST77XX_GREEN);
    delay(150);
    tft.fillScreen(ST77XX_BLUE);
    delay(150);
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
