/*
 * config.h - Receiver (ESP8266 NodeMCU ESP-12E) configuration.
 *
 * IMPORTANT:
 *  - DEVICE_ID and ENCRYPTION_KEY MUST be identical on the Remote and the
 *    Receiver sketches (see Remote/config.h).
 *  - Only receiver.ino includes this header; the nrf_rx.ino tab shares the
 *    same translation unit and sees all constants.
 *
 * NodeMCU pin map (NRF24L01 PA+LNA):
 *   NRF VCC  -> 3.3V (add 10-100 uF cap across VCC/GND)
 *   NRF GND  -> GND
 *   NRF CE   -> D2  (GPIO4)
 *   NRF CSN  -> D8  (GPIO15)
 *   NRF SCK  -> D5  (GPIO14)
 *   NRF MOSI -> D7  (GPIO13)
 *   NRF MISO -> D6  (GPIO12)
 *   NRF IRQ  -> not connected
 *   ESP8266 SPI is hardware-fixed on D5/D6/D7 (SCK/MISO/MOSI), so only
 *   CE and CSN are configurable below.
 */

#pragma once

/* ------------------------------ identity -------------------------------- */

#define DEVICE_ID                0xA1B2u          /* must match Remote        */
#define ENCRYPTION_ENABLED       1
#define ENCRYPTION_KEY_LEN       16
static const uint8_t ENCRYPTION_KEY[ENCRYPTION_KEY_LEN] = {
  0x1C, 0x9B, 0x4F, 0xE7, 0x2A, 0x88, 0xD3, 0x5E,
  0x71, 0x0F, 0xB6, 0xC4, 0x39, 0x6D, 0xA2, 0xF5
};

/* ------------------------- NRF24L01 PA+LNA link --------------------------- */

#define NRF_CE_PIN                4              /* NodeMCU D2 (GPIO4)         */
#define NRF_CSN_PIN               15             /* NodeMCU D8 (GPIO15)        */
#define RF_CHANNEL                108            /* 2.508 GHz                 */
#define RF_DATA_RATE              RF24_250KBPS
#define RF_PA_LEVEL               RF24_PA_HIGH   /* NodeMCU 3.3V rail, PA+LNA  */
#define RF_CRC_LENGTH             RF24_CRC_16
#define RF_RETRY_DELAY            5
#define RF_RETRY_COUNT            15

/* Same address on both ends; must equal Remote/config.h. */
static const uint8_t RECEIVER_ADDRESS[5] = { 0x52, 0x58, 0x31, 0x52, 0x44 }; /* "RX1RD" */

/* --------------------------------- serial --------------------------------- */

#define SERIAL_BAUD               115200
#define DEDUPE_WINDOW             64             /* sliding packet window     */

/* --------------------------------- timing --------------------------------- */

#define HEARTBEAT_INTERVAL_MS     5000           /* heartbeat line to the PC  */
#define LINK_TIMEOUT_MS           15000          /* no packet -> link down    */
