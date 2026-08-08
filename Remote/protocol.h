/*
 * protocol.h - Shared wireless protocol definition for the Spotify Remote system.
 *
 * This header is used by BOTH the Remote and the Receiver firmware. Keep the
 * two copies identical. It must stay free of any Arduino dependency so it can
 * be included from any sketch tab (it only needs <stdint.h> / <stddef.h>).
 *
 * Packet layout (20 bytes, little-endian):
 *
 *   offset  size  field
 *   0       1     magic        = PROTOCOL_MAGIC (not encrypted)
 *   1       2     deviceId     = pairing ID, only trusted device is accepted
 *   3       2     packetNumber = rolling number used for duplicate rejection
 *   5       4     timestampMs  = remote uptime in ms (reboot detection)
 *   9       1     command      = one of Command
 *   10      1     flags        = PacketFlags
 *   11      2     crc16        = CRC-16/XMODEM over bytes 0..10 + 13..18
 *   13      2     fwVersion    = firmware version
 *   15      4     counter      = global monotonic counter (ordering/duplicate)
 *   19      1     checksum     = 8-bit XOR of bytes 0..18
 *
 * Security: bytes 5..18 are obfuscated with a XOR keystream derived from the
 * packet number and the shared key (see applyXorStream). The magic, deviceId
 * and packetNumber stay plaintext so the receiver can drop garbage cheaply.
 * This is obfuscation-grade security (no crypto hardware involved); treat it
 * as "simple packet encryption with a configurable key" as required.
 *
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

#define PROTOCOL_MAGIC    0x5Au
#define PACKET_SIZE       20u

#define FW_MAJOR          1u
#define FW_MINOR          0u
#define FW_PATCH          0u
#define FW_VERSION        ((uint16_t)((FW_MAJOR << 8) | (FW_MINOR << 4) | FW_PATCH))
#define FW_VERSION_STRING "1.0.0"

/* ------------------------------- commands -------------------------------- */

enum Command : uint8_t {
  CMD_NEXT      = 0x01,
  CMD_PREVIOUS  = 0x02,
  CMD_PLAY      = 0x03,
  CMD_VOL_UP    = 0x04,
  CMD_VOL_DOWN  = 0x05,
  CMD_MUTE      = 0x06,
  CMD_HEARTBEAT = 0x07,  /* remote -> receiver keep-alive (every 5 s)        */
  CMD_CONNECT   = 0x08,  /* remote -> receiver link request while searching  */
  CMD_ACK       = 0x09,  /* receiver -> remote acknowledgement payload       */
  CMD_STATUS    = 0x0A,  /* reserved diagnostic command                       */
};

/* --------------------------------- flags --------------------------------- */

enum PacketFlags : uint8_t {
  FLAG_ENCRYPTED = 0x01,
  FLAG_HEARTBEAT = 0x02,
};

/* ------------------------------- packets --------------------------------- */

typedef struct __attribute__((packed)) {
  uint8_t  magic;
  uint16_t deviceId;
  uint16_t packetNumber;
  uint32_t timestampMs;
  uint8_t  command;
  uint8_t  flags;
  uint16_t crc16;
  uint16_t fwVersion;
  uint32_t counter;
  uint8_t  checksum;
} RemotePacket;

typedef struct __attribute__((packed)) {
  uint8_t  command;         /* always CMD_ACK                              */
  uint8_t  linkState;       /* 0 = down, 1 = up (written by the receiver)  */
  uint16_t receiverUptimeS; /* receiver uptime in seconds (diagnostics)    */
} AckPayload;

static_assert(sizeof(RemotePacket) == PACKET_SIZE, "RemotePacket size mismatch");
static_assert(sizeof(AckPayload) == 4, "AckPayload size mismatch");

/* ------------------------- CRC-16 (CCITT/XMODEM) ------------------------- */

static inline uint16_t crc16_update(uint16_t crc, uint8_t byte) {
  crc ^= (uint16_t)byte << 8;
  for (uint8_t b = 0; b < 8; b++) {
    crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
  }
  return crc;
}

static inline uint16_t crc16_xmodem(const uint8_t* data, size_t len) {
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < len; i++) crc = crc16_update(crc, data[i]);
  return crc;
}

static inline uint16_t crc16_update_bytes(uint16_t crc, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) crc = crc16_update(crc, data[i]);
  return crc;
}

/* ------------------------- XOR stream encryption ------------------------- */

/* Mix the packet number, firmware version and key into a 32-bit seed. */
static inline uint32_t stream_seed(uint16_t packetNumber, const uint8_t* key, uint8_t keyLen) {
  uint32_t h = 2166136261u; /* FNV-1a prime start */
  h = (h ^ packetNumber) * 16777619u;
  h = (h ^ FW_VERSION) * 16777619u;
  for (uint8_t i = 0; i < keyLen; i++) h = (h ^ key[i]) * 16777619u;
  /* final avalanche (Thomas Wang style) */
  h ^= h >> 16; h *= 0x85EBCA6Bu;
  h ^= h >> 13; h *= 0xC2B2AE35u;
  h ^= h >> 16;
  return h;
}

/* Deterministic per-byte keystream; identical on both sides. */
static inline uint8_t keystream_byte(uint32_t seed, uint8_t index) {
  uint32_t h = seed ^ ((uint32_t)index * 2654435761u);
  for (uint8_t i = 0; i < 2; i++) {
    h ^= h >> 16; h *= 2246822519u;
    h ^= h >> 13; h *= 3266489917u;
    h ^= h >> 16;
  }
  return (uint8_t)(h >> 24);
}

/*
 * XOR-encrypt/decrypt the protected payload region (bytes 5..18).
 * The seed only depends on fields that are readable BEFORE decryption
 * (packetNumber), so the receiver can decrypt without a chicken-and-egg
 * problem. With ENCRYPTION_ENABLED == 0 the stream is simply never applied.
 */
static inline void applyXorStream(RemotePacket* pkt, const uint8_t* key, uint8_t keyLen) {
  uint32_t seed = stream_seed(pkt->packetNumber, key, keyLen);
  uint8_t* raw = (uint8_t*)pkt;
  for (uint8_t i = 5; i < PACKET_SIZE - 1; i++) {
    raw[i] ^= keystream_byte(seed, i);
  }
}
