/*
 * globals.h - Master shared header for the ESP32 Remote firmware.
 *
 * Design (Arduino Web Editor safe):
 *  - Every source file includes this header, so every shared type, extern
 *    declaration and constant is visible in every translation unit.
 *  - The sketch uses .cpp modules (nrf_comm, buttons, battery, ui) with
 *    explicit .h headers instead of relying on .ino concatenation or
 *    auto-generated prototypes, so tab order can never break the build.
 *  - ALL global variables are declared `extern` here and DEFINED exactly
 *    once, in remote.ino (the main sketch).
 *  - Module function prototypes live in each module's own .h file
 *    (nrf_comm.h, buttons.h, battery.h, ui.h), which include this header
 *    first so types like ScreenState always exist before they are used.
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_sleep.h>

#include "config.h"
#include "protocol.h"

/* ----------------------------- UI screens ------------------------------ */
/* Defined here (not in a .ino) so every header that prototypes a function
 * taking ScreenState can include this file first. */

enum ScreenState {
  SCREEN_BOOT,       /* boot splash with loading animation                    */
  SCREEN_SEARCH,     /* base: looking for the receiver                        */
  SCREEN_CONNECTED,  /* base: home screen (link up)                           */
  SCREEN_ERROR,      /* base: connection lost, retrying                       */
  SCREEN_BTN_ANIM,   /* overlay: button pressed icon animation                */
  SCREEN_BATTERY,    /* overlay: battery info screen                          */
  SCREEN_ABOUT,      /* overlay: about screen                                 */
  SCREEN_SLEEP,      /* overlay: sleep screen -> deep sleep                   */
};

/* ---------------------------- hardware objects --------------------------- */
/* Declared here, DEFINED in remote.ino.                                    */

extern Adafruit_SSD1306 display;
extern RF24 radio;

/* --------------------------- global variables ---------------------------- */
/* All DEFINED in remote.ino (the only place); these externs make them      */
/* visible to every .cpp module.                                            */

extern ScreenState screen;
extern uint32_t    screenUntilMs;      /* when the current overlay expires */
extern uint32_t    connectedSinceMs;   /* when the link last came up       */

extern bool        linkUp;             /* radio link to the receiver alive */
extern bool        everConnected;      /* true once a link ever succeeded  */
extern uint32_t    lastAckMs;          /* last radio ACK received          */
extern uint32_t    lastTxMs;           /* last packet sent                 */
extern uint32_t    lastSearchMs;       /* last CMD_CONNECT attempt         */
extern uint32_t    lastBatteryMs;      /* last battery refresh             */
extern uint32_t    lastActivityMs;     /* last user interaction (sleep)    */
extern uint32_t    lastCommandMs;      /* uptime of the last command       */
extern uint8_t     lastCommand;
extern uint8_t     linkQuality;        /* 0..100 %, TX/ACK success window  */
extern int8_t      batteryPercent;     /* -1 = running on USB              */
extern float       batteryVoltage;
extern uint32_t    packetCounter;      /* global monotonic counter         */
extern uint16_t    packetNumber;       /* rolling number for duplicates    */

extern bool        txPending;          /* a button command awaits delivery */
extern uint8_t     pendingCommand;

#endif /* GLOBALS_H */
