/*
 * nrf_rx.h - Explicit shared declarations for the ESP8266 receive module.
 *
 * Keeping the radio code in nrf_rx.cpp avoids Arduino's automatic .ino
 * prototype generation, which cannot safely order RemotePacket declarations.
 */
#ifndef NRF_RX_H
#define NRF_RX_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

#include "config.h"
#include "protocol.h"

extern RF24 radio;

extern bool     linkUp;
extern uint32_t lastPacketMs;
extern uint32_t lastHeartbeatPrintMs;

extern bool     haveCounter;
extern uint32_t lastCounter;
extern bool     haveTimestamp;
extern uint32_t lastTimestampMs;
extern uint16_t seenNumbers[DEDUPE_WINDOW];
extern uint8_t  seenCount;
extern uint32_t unknownDrops;

void initRadioRx(void);
void updateAckPayload(void);
bool isDuplicate(uint16_t n);
void rememberNumber(uint16_t n);
const char* commandName(uint8_t cmd);
bool validatePacket(RemotePacket* pkt);
void handlePacket(uint8_t pipe, RemotePacket* pkt);

#endif /* NRF_RX_H */
