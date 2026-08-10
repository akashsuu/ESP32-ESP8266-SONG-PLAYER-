/*
 * receiver.ino - Spotify Remote Receiver main sketch.
 *
 * ESP8266 NodeMCU (ESP-12E) + NRF24L01 PA+LNA, connected to the Windows PC
 * over USB (CH340).
 *
 * Tasks:
 *  - listen for validated packets from the paired Remote
 *  - forward media commands to the PC as plain-text lines on Serial
 *  - maintain link state (LINK:UP / LINK:DOWN), ACK payloads and a
 *    watchdog that declares the link lost after LINK_TIMEOUT_MS
 *  - answer PC requests (PING -> PONG)
 *
 * Build settings (Arduino IDE):
 *   Board:  "NodeMCU 1.0 (ESP-12E Module)" (esp8266 core 3.x, via
 *           http://arduino.esp8266.com/stable/package_esp8266com_index.json)
 *   Flash:  Flash Size 4M (1M SPIFFS), 80 MHz, 115200 upload
 *   Libraries: RF24 by TMRh20, version 1.4.6 exactly; SPI is built in.
 *   RF24 1.4.6 includes the ESP8266 2.5.0 pgm_read_ptr() compatibility fix.
 *
 * Wiring (see config.h for the full NodeMCU pin map):
 *   NRF VCC -> 3.3V (+ 10-100 uF across VCC/GND)   NRF CE  -> D2 (GPIO4)
 *   NRF GND -> GND                                  NRF CSN -> D8 (GPIO15)
 *   NRF SCK -> D5 (GPIO14)                          NRF MOSI -> D7 (GPIO13)
 *   NRF MISO -> D6 (GPIO12)                         NRF IRQ -> not connected
 *
 * Serial protocol (one line per message, CR/LF):
 *   READY                     boot acknowledgement
 *   INFO:fw=1.0.0,dev=A1B2    identity at boot
 *   LINK:UP / LINK:DOWN       radio link state changes
 *   NEXT|PREVIOUS|PLAY|VOLUP|VOLDOWN|MUTE          media command
 *   STATUS:batt=94,vol=4112,quality=96             remote battery/link report
 *   SIGNAL:-45                (reserved; remote link quality arrives via STATUS)
 *   HEARTBEAT                 link alive line every 5 s
 *   PONG                      reply to PING
 *   WARN:...                  diagnostics (unknown device, unknown cmd)
 */

#include "nrf_rx.h"

RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);

/* ------------------------------- link state ------------------------------- */

bool     linkUp               = false;
uint32_t lastPacketMs         = 0;
uint32_t lastHeartbeatPrintMs = 0;

/* ---------------------------- anti-replay state ---------------------------- */

bool     haveCounter          = false;
uint32_t lastCounter          = 0;
bool     haveTimestamp        = false;
uint32_t lastTimestampMs      = 0;
uint16_t seenNumbers[DEDUPE_WINDOW];
uint8_t  seenCount            = 0;
uint32_t unknownDrops         = 0;

/* --------------------------------- setup ---------------------------------- */

void setup(void) {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("READY"));
  Serial.print(F("INFO:fw="));
  Serial.print(FW_VERSION_STRING);
  Serial.print(F(",dev="));
  Serial.println(DEVICE_ID, HEX);

  initRadioRx();
  Serial.println(F("INFO:nrf=ok"));
}

/* ---------------------------------- loop ---------------------------------- */

void loop(void) {
  uint32_t now = millis();

  /* Drain the RX FIFO. */
  uint8_t pipe = 0;
  while (radio.available(&pipe)) {
    RemotePacket pkt;
    radio.read(&pkt, sizeof(pkt));
    handlePacket(pipe, &pkt);
  }

  /* Watchdog: no valid packet within LINK_TIMEOUT_MS -> link lost. */
  if (linkUp && now - lastPacketMs >= LINK_TIMEOUT_MS) {
    linkUp = false;
    Serial.println(F("LINK:DOWN"));
    updateAckPayload();
  }

  /* Periodic heartbeat line so the PC app can monitor the link. */
  if (linkUp && now - lastHeartbeatPrintMs >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeatPrintMs = now;
    Serial.println(F("HEARTBEAT"));
  }

  /* PC -> receiver requests. */
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line == F("PING")) {
      Serial.println(F("PONG"));
    }
  }
}
