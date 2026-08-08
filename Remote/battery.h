/*
 * battery.h - battery monitoring and deep sleep management for the Remote.
 *
 * battery.cpp is a separate translation unit, so it includes this header
 * explicitly (which pulls in globals.h).
 */
#ifndef BATTERY_H
#define BATTERY_H

#include "globals.h"

/* LiPo discharge curve: mV -> percent, linear interpolation. */
int8_t percentFromMv(uint32_t mv);

/* Sample the ADC and refresh batteryVoltage / batteryPercent (moving
 * average; percent -1 = running on USB). */
void readBattery(void);

/* Configure the ADC pin and take the first reading. */
void initBattery(void);

/* Throttled refresh (BATTERY_REFRESH_MS). Called from loop(). */
void updateBattery(void);

/* OLED/NRF power down, then ESP32 deep sleep until a button wakes it
 * (EXT1, ANY_LOW - compatible with ESP32 core 2.0.17). Never returns. */
void goToSleep(void);

#endif /* BATTERY_H */
