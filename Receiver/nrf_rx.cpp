/*
 * nrf_rx.cpp - NRF24L01 PA+LNA receive path and packet validation.
 *
 * This is deliberately a .cpp module. It includes nrf_rx.h explicitly so
 * RemotePacket, radio, configuration constants, and shared state are known
 * before any function declaration or definition is compiled.
 */

#include "nrf_rx.h"

void initRadioRx(void) {
  radio.begin();
  radio.setChannel(RF_CHANNEL);
  radio.setDataRate(RF_DATA_RATE);
  radio.setPALevel(RF_PA_LEVEL);
  radio.setAutoAck(true);
  radio.setCRCLength(RF_CRC_LENGTH);
  radio.setRetries(RF_RETRY_DELAY, RF_RETRY_COUNT);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.openReadingPipe(1, RECEIVER_ADDRESS);
  radio.startListening();
}

void updateAckPayload(void) {
  AckPayload ack;
  ack.command = CMD_ACK;
  ack.linkState = linkUp ? 1 : 0;
  ack.receiverUptimeS = (uint16_t)(millis() / 1000);
  radio.writeAckPayload(1, &ack, sizeof(ack));
}

bool isDuplicate(uint16_t n) {
  for (uint8_t i = 0; i < seenCount && i < DEDUPE_WINDOW; i++) {
    if (seenNumbers[i] == n) return true;
  }
  return false;
}

void rememberNumber(uint16_t n) {
  seenNumbers[seenCount % DEDUPE_WINDOW] = n;
  seenCount++;
}

const char* commandName(uint8_t cmd) {
  switch (cmd) {
    case CMD_NEXT:     return "NEXT";
    case CMD_PREVIOUS: return "PREVIOUS";
    case CMD_PLAY:     return "PLAY";
    case CMD_VOL_UP:   return "VOLUP";
    case CMD_VOL_DOWN: return "VOLDOWN";
    case CMD_MUTE:     return "MUTE";
    default:           return "UNKNOWN";
  }
}

bool validatePacket(RemotePacket* pkt) {
  const uint8_t* raw = (const uint8_t*)pkt;

  if (pkt->magic != PROTOCOL_MAGIC) return false;
  if (pkt->deviceId != DEVICE_ID) {
    if (++unknownDrops % 25 == 0) Serial.println(F("WARN:unknown_device"));
    return false;
  }

#if ENCRYPTION_ENABLED
  applyXorStream(pkt, ENCRYPTION_KEY, sizeof(ENCRYPTION_KEY));
#endif

  uint8_t sum = 0;
  for (uint8_t i = 0; i < PACKET_SIZE - 1; i++) sum ^= raw[i];
  if (sum != pkt->checksum) return false;

  uint16_t crc = crc16_xmodem(raw, 11);
  crc = crc16_update_bytes(crc, raw + 13, 6);
  if (crc != pkt->crc16) return false;

  if (haveTimestamp && (int32_t)(pkt->timestampMs - lastTimestampMs) < -60000) {
    haveCounter = false;
    haveTimestamp = false;
    seenCount = 0;
  }

  if (haveCounter && (int32_t)(pkt->counter - lastCounter) <= 0) return false;
  haveCounter = true;
  lastCounter = pkt->counter;
  lastTimestampMs = pkt->timestampMs;
  haveTimestamp = true;

  if (isDuplicate(pkt->packetNumber)) return false;
  rememberNumber(pkt->packetNumber);
  return true;
}

void handlePacket(uint8_t pipe, RemotePacket* pkt) {
  if (!validatePacket(pkt)) return;

  lastPacketMs = millis();
  switch (pkt->command) {
    case CMD_CONNECT:
    case CMD_HEARTBEAT:
      if (!linkUp) {
        linkUp = true;
        Serial.println(F("LINK:UP"));
      }
      break;

    case CMD_NEXT:
    case CMD_PREVIOUS:
    case CMD_PLAY:
    case CMD_VOL_UP:
    case CMD_VOL_DOWN:
    case CMD_MUTE:
      Serial.println(commandName(pkt->command));
      break;

    case CMD_STATUS:
      /* Retained for compatibility with older remotes. */
      Serial.println(F("STATUS:legacy"));
      break;

    default:
      Serial.print(F("WARN:unknown_cmd="));
      Serial.println(pkt->command, HEX);
      break;
  }

  updateAckPayload();
  (void)pipe;
}
