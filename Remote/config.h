/*
 * config.h - Remote (ESP32-WROOM-DA) configuration.
 *
 * IMPORTANT:
 *  - DEVICE_ID and ENCRYPTION_KEY MUST be identical on the Remote and the
 *    Receiver sketches. Change them once before deployment and flash both.
 *  - This header is included by globals.h, which every .ino tab includes,
 *    so every constant below is visible in every tab no matter how the
 *    Arduino Web Editor orders them.
 *  - Do NOT change RF_CHANNEL / RECEIVER_ADDRESS / DEVICE_ID / the key:
 *    they must keep matching Receiver/config.h for the link to work.
 */

#pragma once

/* ------------------------------ identity -------------------------------- */

#define DEVICE_NAME              "Spotify Remote"
#define DEVICE_ID                0xA1B2u          /* must match Receiver       */
#define ENCRYPTION_ENABLED       1
#define ENCRYPTION_KEY_LEN       16
static const uint8_t ENCRYPTION_KEY[ENCRYPTION_KEY_LEN] = {
  0x1C, 0x9B, 0x4F, 0xE7, 0x2A, 0x88, 0xD3, 0x5E,
  0x71, 0x0F, 0xB6, 0xC4, 0x39, 0x6D, 0xA2, 0xF5
};

/* ------------------------- NRF24L01 PA+LNA link --------------------------- */

#define NRF_CE_PIN                4
#define NRF_CSN_PIN               5
#define RF_CHANNEL                108            /* 2.508 GHz                 */
#define RF_DATA_RATE              RF24_250KBPS   /* maximum range             */
#define RF_PA_LEVEL               RF24_PA_MAX
#define RF_CRC_LENGTH             RF24_CRC_16
#define RF_RETRY_DELAY            5              /* 5 x 250 us = 1.25 ms       */
#define RF_RETRY_COUNT            15             /* total attempts             */

/* Same address on both ends; byte 0 must never be 0x00. */
static const uint8_t RECEIVER_ADDRESS[5] = { 0x52, 0x58, 0x31, 0x52, 0x44 }; /* "RX1RD" */

/* ------------------------------ OLED display ------------------------------ */

#define SCREEN_WIDTH              128
#define SCREEN_HEIGHT             64
#define OLED_RESET_PIN            -1
#define OLED_I2C_ADDR             0x3Cu
#define OLED_SDA_PIN              21
#define OLED_SCL_PIN              22
#define OLED_POWER_PIN            2              /* GPIO powering OLED VCC;
                                                    -1 = always on            */
#define OLED_REFRESH_MS           80             /* ~12 fps framebuffer draw  */

/* -------------------------------- buttons -------------------------------- */
/* Buttons are active-low and use the ESP32 internal pull-ups.                */

#define BTN_NEXT                  32
#define BTN_PREVIOUS              33
#define BTN_PLAY                  25
#define BTN_VOL_UP                26
#define BTN_VOL_DOWN              27
#define BTN_MUTE                  14

#define BUTTON_DEBOUNCE_MS        25
#define LONG_PRESS_MS             1000           /* long press action         */
#define REPEAT_START_MS           500            /* hold-to-repeat start      */
#define REPEAT_INTERVAL_MS        150            /* hold-to-repeat rate       */

#define LONG_ACTION_NONE          0
#define LONG_ACTION_ABOUT         1              /* long press Play/Pause     */

/* --------------------------------- timing --------------------------------- */

#define HEARTBEAT_INTERVAL_MS     5000           /* keep-alive packet period   */
#define LINK_TIMEOUT_MS           15000          /* no ACK for 15 s -> lost    */
#define SEARCH_RETRY_MS           500            /* connect request while down */
#define CONNECTED_SCREEN_MS       2000           /* "Receiver Connected" splash*/
#define BUTTON_SCREEN_MS          1000           /* button icon animation      */
#define STATUS_SCREEN_MS          4000           /* about overlay              */
