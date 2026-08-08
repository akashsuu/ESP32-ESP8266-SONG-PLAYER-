/*
 * nrf_rx.ino - NRF24L01 PA+LNA receive path and packet validation.
 *
 * Validation pipeline (cheap checks first, expensive last):
 *   1. magic byte                        -> drop non-protocol traffic
 *   2. deviceId                          -> drop unknown devices (pairing)
 *   3. decrypt protected payload region  -> XOR stream with shared key
 *   4. XOR checksum                      -> drop corrupted frames
 *   5. CRC-16                            -> drop corrupted/unauthorised data
 *   6. counter + timestamp monotonicity  -> drop replays / reordering
 *   7. packet number sliding window      -> drop duplicates
 *
 * Valid media commands are printed on Serial as bare command names
 * (NEXT, PREVIOUS, ...) for the PC app; every valid packet also refreshes
 * the ACK payload the remote reads after its TX.
 */

/* --------------------------------- init ----------------------------------- */

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

/* ------------------------------ ack payload ------------------------------- */

void updateAckPayload(void) {
  AckPayload ack;
  ack.command         = CMD_ACK;
  ack.linkState       = linkUp ? 1 : 0;
  ack.receiverUptimeS = (uint16_t)(millis() / 1000);
  radio.writeAckPayload(1, &ack, sizeof(ack));
}

/* ---------------------------- duplicate window ----------------------------- */

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

/* ----------------------------- command names ------------------------------ */

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

/* ------------------------------ validation -------------------------------- */

bool validatePacket(RemotePacket* pkt) {
  const uint8_t* raw = (const uint8_t*)pkt;

  if (pkt->magic != PROTOCOL_MAGIC) return false;

  /* Unknown device -> reject (pairing). */
  if (pkt->deviceId != DEVICE_ID) {
    if (++unknownDrops % 25 == 0) Serial.println(F("WARN:unknown_device"));
    return false;
  }

#if ENCRYPTION_ENABLED
  applyXorStream(pkt, ENCRYPTION_KEY, sizeof(ENCRYPTION_KEY));
#endif

  /* XOR checksum over bytes 0..18. */
  uint8_t sum = 0;
  for (uint8_t i = 0; i < PACKET_SIZE - 1; i++) sum ^= raw[i];
  if (sum != pkt->checksum) return false;

  /* CRC-16 over everything except the crc16 field and the checksum byte. */
  uint16_t crc = crc16_xmodem(raw, 11);
  crc = crc16_update_bytes(crc, raw + 13, 6);
  if (crc != pkt->crc16) return false;

  /* Reboot detection: remote uptime went backwards by more than a minute. */
  if (haveTimestamp && (int32_t)(pkt->timestampMs - lastTimestampMs) < -60000) {
    haveCounter = false;
    haveTimestamp = false;
    seenCount = 0;
  }

  /* The monotonic counter rejects replays and reordered frames. */
  if (haveCounter && (int32_t)(pkt->counter - lastCounter) <= 0) return false;
  haveCounter = true;
  lastCounter = pkt->counter;
  lastTimestampMs = pkt->timestampMs;
  haveTimestamp = true;

  /* Sliding window rejects duplicates (belt and braces). */
  if (isDuplicate(pkt->packetNumber)) return false;
  rememberNumber(pkt->packetNumber);

  return true;
}

/* ------------------------------- dispatch --------------------------------- */

void handlePacket(uint8_t pipe, RemotePacket* pkt) {
  if (!validatePacket(pkt)) return;

  lastPacketMs = millis();

  switch (pkt->command) {
    case CMD_CONNECT:
      if (!linkUp) {
        linkUp = true;
        Serial.println(F("LINK:UP"));
      }
      break;

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

    case CMD_STATUS: {
      /* Battery + link quality report (packing defined in protocol.h).
       * batt is stored as 0..100, or 255 when the remote runs on USB.
       * quality is the remote's estimated link quality 0..100 (TX/ACK
       * success) - the NRF24L01 has no real RSSI to report. */
      uint32_t mv   = pkt->timestampMs;
      uint8_t  batt = (uint8_t)((pkt->fwVersion >> 8) & 0xFF);
      uint8_t  quality = (uint8_t)(pkt->fwVersion & 0xFF);
      Serial.print(F("STATUS:batt="));
      Serial.print(batt);
      Serial.print(F(",vol="));
      Serial.print(mv);
      Serial.print(F(",quality="));
      Serial.println(quality);
      break;
    }

    default:
      Serial.print(F("WARN:unknown_cmd="));
      Serial.println(pkt->command, HEX);
      break;
  }

  updateAckPayload();
  (void)pipe;
}
