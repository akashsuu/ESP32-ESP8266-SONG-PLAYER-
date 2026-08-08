/*
 * battery.cpp - battery monitoring and deep sleep management for the Remote.
 *
 * The battery voltage is measured through a 100k/100k divider on GPIO36
 * (ADC1_CH6). A moving average filters the readings. If the voltage is
 * implausible (below BAT_MIN_PLAUSIBLE_MV) the remote assumes it runs on
 * USB and reports percent = -1 ("Power USB").
 *
 * Deep sleep: OLED and NRF are powered down (optionally cut off via
 * OLED_POWER_PIN / NRF_POWER_PIN MOSFETs), then the ESP32 sleeps until any
 * of the six buttons is pressed (EXT1 wake, ANY_LOW). setup() runs again
 * after wake. ESP_EXT1_WAKEUP_ANY_LOW is available in ESP32 Arduino core
 * 2.0.17 (and 3.x) - the old legacy EXT1 wakeup constant from earlier
 * core releases is NOT available.
 *
 * This is a .cpp translation unit: it includes battery.h explicitly and
 * relies on globals.h for all shared state - never on .ino tab order.
 */

#include "battery.h"

/* ----------------------------- voltage -> % ------------------------------- */

/* LiPo discharge curve points: {mV, percent}, linear interpolation. */
static const int16_t BAT_CURVE[][2] = {
  { 3100, 0 }, { 3300, 10 }, { 3450, 20 }, { 3550, 30 }, { 3650, 40 },
  { 3750, 50 }, { 3850, 60 }, { 3950, 70 }, { 4050, 80 }, { 4150, 90 },
  { 4200, 100 },
};

int8_t percentFromMv(uint32_t mv) {
  if (mv >= BAT_CURVE[10][0]) return 100;
  for (uint8_t i = 1; i <= 10; i++) {
    if (mv < (uint32_t)BAT_CURVE[i][0]) {
      int16_t m0 = BAT_CURVE[i - 1][0], m1 = BAT_CURVE[i][0];
      int16_t p0 = BAT_CURVE[i - 1][1], p1 = BAT_CURVE[i][1];
      return (int8_t)(p0 + (int32_t)(mv - m0) * (p1 - p0) / (m1 - m0));
    }
  }
  return 100;
}

/* ------------------------------ measurement ------------------------------- */

static bool   haveEma = false;
static float  emaMv   = 0.0f;

void readBattery(void) {
  uint32_t sum = 0;
  const uint8_t SAMPLES = 8;
  for (uint8_t i = 0; i < SAMPLES; i++) {
    sum += analogReadMilliVolts(BAT_ADC_PIN);
  }
  uint32_t adcMv = sum / SAMPLES;
  uint32_t batMv = (uint32_t)((float)adcMv * BAT_DIVIDER_RATIO);

  if (!haveEma) {
    emaMv = (float)batMv;
    haveEma = true;
  } else {
    emaMv = emaMv * 0.8f + (float)batMv * 0.2f;
  }

  batteryVoltage = emaMv / 1000.0f;
  batteryPercent = (emaMv < BAT_MIN_PLAUSIBLE_MV) ? (int8_t)-1
                                                  : percentFromMv((uint32_t)emaMv);
}

void initBattery(void) {
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
  adcAttachPin(BAT_ADC_PIN);
#endif
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_ATTEN_DB_11);
#else
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);
#endif
  readBattery();
}

void updateBattery(void) {
  uint32_t now = millis();
  if (now - lastBatteryMs >= BATTERY_REFRESH_MS) {
    lastBatteryMs = now;
    readBattery();
  }
}

/* ------------------------------- deep sleep ------------------------------- */

void goToSleep(void) {
  /* Last frame is already on the display; switch everything off. */
  if (OLED_POWER_PIN >= 0) {
    digitalWrite(OLED_POWER_PIN, LOW);
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

  radio.powerDown();               /* nRF24 power-down mode                */

#if NRF_POWER_PIN >= 0
  pinMode(NRF_POWER_PIN, OUTPUT);
  digitalWrite(NRF_POWER_PIN, LOW);  /* cut the NRF supply with a MOSFET  */
#endif

  /* Wake on any button press (all pins are RTC-capable). Buttons are
   * active LOW, so ANY_LOW wakes on any press. ESP_EXT1_WAKEUP_ANY_LOW
   * exists in core 2.0.17 (and 3.x). */
  esp_sleep_enable_ext1_wakeup(BUTTON_WAKE_MASK, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_deep_sleep_start();          /* never returns; setup() runs on wake */
}
