#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "config.h"
#include "camera_driver.h"
#include "display_driver.h"
#include "bt_transfer.h"

// System States
enum AppState {
    STATE_BT_WAITING,
    STATE_BT_CONNECTED,
    STATE_CAMERA_PREVIEW,
    STATE_PHOTO_CAPTURE
};

static AppState currentState = STATE_BT_WAITING;

// Button Debounce Management
static int lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;

void setup() {
    // 1. Disable ESP32 Brownout Detector (Prevents power-dip reboot loops)
    WRITE_PERI_REG(RTC_CNTL_BROWNOUT_REG, 0);

    Serial.begin(115200);
    delay(300);
    Serial.println("\n==========================================");
    Serial.println("   ESP32 Standalone Camera Initializing   ");
    Serial.println("==========================================");

    // 2. Setup Shutter Button
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // 3. Initialize ST7735 TFT Display
    Serial.println("[1/3] Initializing ST7735 TFT Display...");
    initDisplay();
    showWaitingScreen();
    Serial.println("      -> TFT Display Ready!");

    // 4. Power Stabilization Delay
    delay(200);

    // 5. Initialize Bluetooth SPP Advertising
    Serial.println("[2/3] Initializing Bluetooth SPP...");
    if (!initBluetooth()) {
        Serial.println("      -> ERROR: Bluetooth init failed!");
    } else {
        Serial.println("      -> SUCCESS: Advertising as 'ESP32-CAMERA'");
    }

    // 6. Initialize OV7670 Camera Hardware
    Serial.println("[3/3] Attempting OV7670 Camera Init...");
    if (!initCamera()) {
        Serial.println("      -> NOTICE: OV7670 Camera not detected (Screen active for standalone testing)");
    } else {
        Serial.println("      -> SUCCESS: OV7670 Camera Initialized");
    }
}

void loop() {
    switch (currentState) {
        case STATE_BT_WAITING: {
            if (isBluetoothConnected()) {
                Serial.println("Bluetooth Client Connected!");
                currentState = STATE_BT_CONNECTED;
                showConnectedScreen();
                delay(1000); // Display CONNECTED splash screen for 1 second
                currentState = STATE_CAMERA_PREVIEW;
            }
            break;
        }

        case STATE_BT_CONNECTED: {
            // Transitional state handled in loop above
            break;
        }

        case STATE_CAMERA_PREVIEW: {
            // Check for Bluetooth disconnection
            if (!isBluetoothConnected()) {
                Serial.println("Bluetooth Client Disconnected. Returning to waiting screen.");
                showWaitingScreen();
                currentState = STATE_BT_WAITING;
                break;
            }

            // Capture and render live camera preview frame
            camera_fb_t* fb = captureFrame();
            if (fb) {
                renderCameraFrame(fb);
                releaseFrame(fb);
            }

            // Button Debounce Check
            int reading = digitalRead(BUTTON_PIN);
            if (reading != lastButtonState) {
                lastDebounceTime = millis();
                lastButtonState = reading;
            }

            if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
                if (reading == LOW) { // Button pressed
                    Serial.println("Shutter Button Pressed! Triggering Photo Capture...");
                    currentState = STATE_PHOTO_CAPTURE;
                    // Wait for button release to prevent multiple triggers
                    while (digitalRead(BUTTON_PIN) == LOW) {
                        delay(10);
                    }
                }
            }
            break;
        }

        case STATE_PHOTO_CAPTURE: {
            // Capture frozen frame for transmission
            camera_fb_t* fb = captureFrame();
            if (fb) {
                Serial.println("Photo captured. Streaming frame over Bluetooth SPP...");
                bool success = sendPhotoFrame(fb);
                releaseFrame(fb);

                if (success) {
                    Serial.println("Photo successfully sent over Bluetooth!");
                } else {
                    Serial.println("Photo transmission failed or interrupted.");
                }
            } else {
                Serial.println("Failed to capture frame from OV7670!");
            }

            // Resume live camera preview
            currentState = STATE_CAMERA_PREVIEW;
            break;
        }
    }

    vTaskDelay(pdMS_TO_TICKS(5)); // Yield to prevent WDT trigger
}
