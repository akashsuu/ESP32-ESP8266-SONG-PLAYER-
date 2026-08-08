/*
 * nrf_comm.h - NRF24L01 PA+LNA transmit path for the Remote.
 *
 * This header is explicit on purpose: nrf_comm.cpp is a separate
 * translation unit, so it must include this header (which pulls in
 * globals.h) rather than relying on .ino concatenation order.
 */
#ifndef NRF_COMM_H
#define NRF_COMM_H

#include "globals.h"

/* Initialise the radio (channel 108, 250 kbps, PA max, CRC-16, auto-ACK,
 * retries, dynamic payloads, ACK payloads). */
void initRadio(void);

/* CRC over everything except the crc16 field itself and the checksum byte. */
uint16_t packetCrc(const RemotePacket* p);

/* XOR checksum over bytes 0..18. */
uint8_t packetChecksum(const RemotePacket* p);

/* Build, protect and transmit one packet. Returns true when the radio
 * layer received an ACK (write() succeeded). Also updates linkQuality. */
bool sendPacket(uint8_t cmd);

#endif /* NRF_COMM_H */
