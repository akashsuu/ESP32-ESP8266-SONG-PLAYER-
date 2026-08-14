#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>

// Format identifiers
#define IMAGE_FMT_RGB565  0x01
#define IMAGE_FMT_JPEG    0x02

// Magic Markers
#define START_MAGIC_0     0xAA
#define START_MAGIC_1     0xBB
#define START_MAGIC_2     0xCC
#define START_MAGIC_3     0xDD

#define PKT_MAGIC_0       0x55
#define PKT_MAGIC_1       0xAA

#define END_MAGIC_0       0xDE
#define END_MAGIC_1       0xAD
#define END_MAGIC_2       0xBE
#define END_MAGIC_3       0xEF

#pragma pack(push, 1)

// 24-byte Photo Stream Header
struct PhotoHeader {
    uint8_t  magic[4];       // 0xAA, 0xBB, 0xCC, 0xDD
    uint8_t  format;         // 0x01 = RGB565, 0x02 = JPEG
    uint16_t width;          // Image width in pixels (e.g. 160)
    uint16_t height;         // Image height in pixels (e.g. 120)
    uint32_t image_size;     // Total bytes of raw image data (e.g. 38400)
    uint16_t total_packets;  // Total number of payload data packets
    uint8_t  checksum;       // Header XOR checksum
    uint8_t  reserved[6];    // Padding for alignment / future use
};

// 7-byte Packet Header (precedes payload data)
struct PacketHeader {
    uint8_t  magic[2];       // 0x55, 0xAA
    uint16_t packet_num;     // 0..total_packets-1
    uint16_t payload_len;    // Number of payload bytes in this chunk
    uint8_t  checksum;       // XOR checksum of payload bytes
};

// 4-byte End-of-Photo Marker
struct PhotoFooter {
    uint8_t  magic[4];       // 0xDE, 0xAD, 0xBE, 0xEF
};

#pragma pack(pop)

#endif // PROTOCOL_H
