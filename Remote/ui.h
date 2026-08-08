/*
 * ui.h - SSD1306 OLED interface: all screens, icons and animations.
 *
 * ScreenState is defined in globals.h; this header includes it FIRST so
 * every prototype below is only declared after the type exists. ui.cpp is
 * a separate translation unit and includes this header explicitly.
 */
#ifndef UI_H
#define UI_H

#include "globals.h"

/* Human-readable name for a media command (plain string literals). */
const char* commandName(uint8_t cmd);

/* True for overlay screens that auto-return to the base screen. */
bool isTransient(ScreenState s);

/* Initialise the display (power pin, I2C at 400 kHz, SSD1306 begin). */
void initDisplay(void);

/* ---- draw helpers ---- */
void drawSignalBars(int x, int y, int8_t percent);
void drawSpeaker(int cx, int cy);
void drawIcon(uint8_t cmd, int cx, int cy);

/* ---- screens ---- */
void drawBootScreen(uint32_t now);
void drawSearchScreen(uint32_t now);
void drawHomeScreen(uint32_t now);
void drawButtonScreen(uint32_t now);
void drawErrorScreen(uint32_t now);
void drawAboutScreen(uint32_t now);

/* Throttled frame router - called from updateScreen(). */
void drawFrame(void);

#endif /* UI_H */
