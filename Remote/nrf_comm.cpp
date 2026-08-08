/*
 * nrf_comm.cpp - NRF24L01 PA+LNA transmit path for the Remote.
 *
 * Responsibilities:
 *  - initialise the radio (channel 108, 250 kbps, PA max, CRC-16,
 *    auto-ACK, retries, dynamic payloads, ACK payloads)
 *  - build, sign and encrypt a RemotePacket
 *  - transmit with radio-level auto retry and read the ACK payload
 *  - estimate link quality from recent TX/ACK success (the NRF24L01 has
 *    no RSSI on this RF24 build, so we do not pretend it does)
 *
 * This is a .cpp translation unit: it includes nrf_comm.h explicitly and
 * relies on globals.h for all shared state - never on .ino tab order.
 */

#include "nrf_comm.h"

/* ------------------------------ link quality ------------------------------ */
/* Rolling window over the last LINK_QUALITY_WINDOW transmissions. Every
 * radio.write() that returned true means the receiver ACKed at the radio
 * level, so acked/window is a fair estimate of link health. */

#define LINK_QUALITY_WINDOW 32

static uint8_t linkWindow[LINK_QUALITY_WINDOW];
static uint8_t linkWindowIndex = 0;
static uint8_t linkWindowAcked = 0;
static uint8_t linkWindowUsed  = 0;

static void recordLinkResult(bool acked) {
  if (linkWindowUsed < LINK_QUALITY_WINDOW) {
    linkWindowUsed++;
  } else {
    linkWindowAcked -= linkWindow[linkWindowIndex];
  }
  linkWindow[linkWindowIndex] = acked ? 1 : 0;
  linkWindowAcked += acked ? 1 : 0;
  linkWindowIndex = (uint8_t)((linkWindowIndex + 1) % LINK_QUALITY_WINDOW);
  linkQuality = (uint8_t)((uint16_t)linkWindowAcked * 100 / linkWindowUsed);
}

/* -------------------------------- init ------------------------------------ */

void initRadio(void) {
  radio.begin();
  radio.setChannel(RF_CHANNEL);
  radio.setDataRate(RF_DATA_RATE);
  radio.setPALevel(RF_PA_LEVEL);
  radio.setAutoAck(true);
  radio.setCRCLength(RF_CRC_LENGTH);
  radio.setRetries(RF_RETRY_DELAY, RF_RETRY_COUNT);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.openWritingPipe(RECEIVER_ADDRESS);
  radio.stopListening();
  delay(20); /* one-time init settle */
}

/* ------------------------------ packet helpers ---------------------------- */

/* CRC over everything except the crc16 field itself and the checksum byte. */
uint16_t packetCrc(const RemotePacket* p) {
  const uint8_t* raw = (const uint8_t*)p;
  uint16_t crc = crc16_xmodem(raw, 11);          /* bytes 0..10              */
  return crc16_update_bytes(crc, raw + 13, 6);   /* bytes 13..18             */
}

/* XOR checksum over bytes 0..18. */
uint8_t packetChecksum(const RemotePacket* p) {
  const uint8_t* raw = (const uint8_t*)p;
  uint8_t sum = 0;
  for (uint8_t i = 0; i < PACKET_SIZE - 1; i++) sum ^= raw[i];
  return sum;
}

/* -------------------------------- send ------------------------------------ */

/*
 * Build, protect and transmit one packet. Returns true when the radio
 * layer received an ACK (write() succeeded), which also refreshes
 * lastAckMs / linkUp. linkQuality is updated from every TX outcome.
 */
bool sendPacket(uint8_t cmd) {
  RemotePacket pkt;
  memset(&pkt, 0, sizeof(pkt));

  pkt.magic        = PROTOCOL_MAGIC;
  pkt.deviceId     = DEVICE_ID;
  pkt.packetNumber = ++packetNumber;
  pkt.timestampMs  = millis();
  pkt.command      = cmd;
  pkt.flags        = 0;
  pkt.fwVersion    = FW_VERSION;
  pkt.counter      = ++packetCounter;

#if ENCRYPTION_ENABLED
  pkt.flags |= FLAG_ENCRYPTED;
#endif
  if (cmd == CMD_HEARTBEAT) pkt.flags |= FLAG_HEARTBEAT;

  if (cmd == CMD_STATUS) {
    /* Battery + link quality report (see protocol.h for the packing).
     * The NRF24L01 provides no real RSSI, so we report the estimated
     * link quality (0..100 %) in place of the old rssi byte. */
    pkt.timestampMs = (uint32_t)(batteryVoltage * 1000.0f);
    uint16_t info = (uint16_t)((uint8_t)(batteryPercent >= 0 ? batteryPercent : 255)) << 8;
    info |= (uint8_t)(linkQuality & 0xFF);
    pkt.fwVersion = info;
  }

  pkt.crc16    = packetCrc(&pkt);   /* plaintext integrity                  */
  pkt.checksum = packetChecksum(&pkt);

#if ENCRYPTION_ENABLED
  applyXorStream(&pkt, ENCRYPTION_KEY, sizeof(ENCRYPTION_KEY));
#endif

  radio.stopListening();
  bool ok = radio.write(&pkt, sizeof(pkt));

  /* Read the receiver's status ACK payload (linkState + receiver uptime). */
  AckPayload ack;
  if (radio.isAckPayloadAvailable()) {
    radio.read(&ack, sizeof(ack));
  }

  /* Link quality: every successful write() was acknowledged at the
   * radio level, every failed write() was not. */
  recordLinkResult(ok);

  if (ok) {
    lastAckMs = millis();
    if (!linkUp) {
      linkUp = true;
      everConnected = true;
      connectedSinceMs = lastAckMs;
      screen = SCREEN_CONNECTED;
      screenUntilMs = lastAckMs + CONNECTED_SCREEN_MS;
    }
  }
  return ok;
}
