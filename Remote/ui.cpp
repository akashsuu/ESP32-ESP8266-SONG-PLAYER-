/*
 * ui.cpp - SSD1306 OLED interface: all screens, icons and animations.
 *
 * One 1 KB framebuffer is drawn at most every OLED_REFRESH_MS (80 ms),
 * which keeps the I2C bus load low and leaves CPU time for the RF link.
 * All strings are kept in flash with F() to save RAM.
 *
 * isTransient() lives here (not in remote.ino) so that no auto-generated
 * .ino prototype can ever conflict with ScreenState.
 *
 * This is a .cpp translation unit: it includes ui.h explicitly and
 * relies on globals.h for all shared state - never on .ino tab order.
 */

#include <string.h>
#include "ui.h"

/* --------------------------- common text helpers -------------------------- */

/* Plain string literals on purpose: returning F() strings from a function
 * declared `const char*` is a type error (F() yields __FlashStringHelper*).
 * On the ESP32 string literals live in the memory-mapped flash anyway. */
const char* commandName(uint8_t cmd) {
  switch (cmd) {
    case CMD_NEXT:     return "Next";
    case CMD_PREVIOUS: return "Previous";
    case CMD_PLAY:     return "Play";
    case CMD_VOL_UP:   return "Vol +";
    case CMD_VOL_DOWN: return "Vol -";
    case CMD_MUTE:     return "Mute";
    default:           return "---";
  }
}

/* Overlay screens (and the boot splash) automatically return to the base
 * screen after their screenUntilMs deadline. The base screen follows the
 * link state: connected -> home, lost after being up -> error, never up
 * -> searching. */
bool isTransient(ScreenState s) {
  return s == SCREEN_BOOT || s == SCREEN_BTN_ANIM || s == SCREEN_BATTERY || s == SCREEN_ABOUT;
}

/* ------------------------------- init ------------------------------------- */

void initDisplay(void) {
  if (OLED_POWER_PIN >= 0) {
    pinMode(OLED_POWER_PIN, OUTPUT);
    digitalWrite(OLED_POWER_PIN, HIGH);
  }
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  display.clearDisplay();
  display.display();
}

/* ------------------------------ draw helpers ------------------------------ */

void drawBatteryIcon(int x, int y, int8_t percent) {
  display.drawRect(x, y, 12, 6, SSD1306_WHITE);
  display.fillRect(x + 12, y + 2, 2, 2, SSD1306_WHITE);
  int fill = (percent * 10) / 100;
  if (fill > 0) display.fillRect(x + 1, y + 1, fill, 4, SSD1306_WHITE);
}

/* Five signal bars driven by the estimated link quality (0..100 %). */
void drawSignalBars(int x, int y, int8_t percent) {
  if (!linkUp) return;   /* no bars while disconnected */
  for (int i = 0; i < 5; i++) {
    int h = 3 + i * 2;
    int xo = x + i * 5;
    if (percent >= (i + 1) * 20) {
      display.fillRect(xo, y + 8 - h, 3, h, SSD1306_WHITE);
    } else {
      display.drawRect(xo, y + 8 - h, 3, h, SSD1306_WHITE);
    }
  }
}

/* Speaker body shared by the volume icons. */
void drawSpeaker(int cx, int cy) {
  display.fillTriangle(cx - 12, cy - 6, cx - 12, cy + 6, cx - 4, cy + 6, SSD1306_WHITE);
  display.fillRect(cx - 12, cy - 6, 8, 12, SSD1306_WHITE);
}

/* Large centered icon for the button animation screen. */
void drawIcon(uint8_t cmd, int cx, int cy) {
  const int h = 12;
  switch (cmd) {
    case CMD_NEXT:
      display.fillRect(cx - 16, cy - h, 4, 2 * h, SSD1306_WHITE);
      display.fillTriangle(cx - 10, cy - h, cx + 10, cy, cx - 10, cy + h, SSD1306_WHITE);
      break;
    case CMD_PREVIOUS:
      display.fillTriangle(cx - 10, cy - h, cx + 10, cy - h, cx + 10, cy + h, SSD1306_WHITE);
      display.fillRect(cx + 12, cy - h, 4, 2 * h, SSD1306_WHITE);
      break;
    case CMD_PLAY:
      display.fillTriangle(cx - 8, cy - h, cx + 10, cy, cx - 8, cy + h, SSD1306_WHITE);
      break;
    case CMD_VOL_UP:
      drawSpeaker(cx - 4, cy);
      display.fillRect(cx + 6, cy - 2, 8, 4, SSD1306_WHITE);
      display.fillRect(cx + 8, cy - 4, 4, 8, SSD1306_WHITE);
      break;
    case CMD_VOL_DOWN:
      drawSpeaker(cx - 4, cy);
      display.fillRect(cx + 6, cy - 2, 8, 4, SSD1306_WHITE);
      break;
    case CMD_MUTE:
      drawSpeaker(cx - 4, cy);
      display.drawLine(cx + 2, cy - 6, cx + 14, cy + 6, SSD1306_WHITE);
      display.drawLine(cx + 2, cy + 6, cx + 14, cy - 6, SSD1306_WHITE);
      break;
    default:
      display.fillTriangle(cx - 8, cy - h, cx + 10, cy, cx - 8, cy + h, SSD1306_WHITE);
      break;
  }
}

/* ------------------------------- screens ---------------------------------- */

/* Boot splash with a looping progress bar. */
void drawBootScreen(uint32_t now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(8, 4);  display.println(F("Spotify Remote"));
  display.setCursor(8, 15); display.println(F("ESP32-WROOM-DA"));
  display.setCursor(8, 26); display.println(F("Initializing..."));
  display.setCursor(8, 37); display.print(F("FW ")); display.println(FW_VERSION_STRING);
  uint16_t w = (uint16_t)((now % 1600) * 112 / 1600);
  display.drawRect(8, 50, 112, 8, SSD1306_WHITE);
  if (w > 0) display.fillRect(8, 50, w, 8, SSD1306_WHITE);
}

/* Searching screen with an animated dot row. */
void drawSearchScreen(uint32_t now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(20, 12); display.println(F("Searching..."));
  display.setCursor(8, 26);  display.println(F("Looking for"));
  display.setCursor(8, 36);  display.println(F("Receiver"));
  uint8_t dots = (now / 200) % 4;
  display.setCursor(40, 50);
  for (uint8_t i = 0; i < 3; i++) display.print(i < dots ? '.' : ' ');
}

/* Home screen: link quality, battery, last command, idle time, FW. */
void drawHomeScreen(uint32_t now) {
  display.clearDisplay();

  /* Inverted header bar. */
  display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(2, 2);
  display.print(F("Spotify Remote"));

  display.setTextColor(SSD1306_WHITE);
  int y = 12;

  if (now - connectedSinceMs < CONNECTED_SCREEN_MS && linkUp) {
    /* Brief "Receiver Connected / Ready" splash. */
    display.setCursor(6, y + 6);  display.println(F("Receiver Connected"));
    display.setCursor(6, y + 16); display.println(F("Ready"));
    drawSignalBars(96, y + 6, (int8_t)linkQuality);
    display.display();
    return;
  }

  /* Row 1: estimated link quality (TX/ACK success, not RSSI). */
  display.setCursor(2, y);
  if (linkUp) {
    display.print(F("Link: "));
    display.print(linkQuality);
    display.print('%');
  } else {
    display.print(everConnected ? F("Connection Lost") : F("Searching..."));
  }
  drawSignalBars(100, y, (int8_t)linkQuality);
  y += 10;

  /* Row 2: battery percentage + icon. */
  display.setCursor(2, y);
  if (batteryPercent >= 0) {
    display.print(F("Battery "));
    display.print(batteryPercent);
    display.print('%');
    drawBatteryIcon(100, y, batteryPercent);
  } else {
    display.print(F("Power USB"));
  }
  y += 10;

  /* Row 3: last button pressed. */
  display.setCursor(2, y);
  display.print(F("Last: "));
  display.print(commandName(lastCommand));
  y += 10;

  /* Row 4: time since last command + firmware version. */
  display.setCursor(2, y);
  uint32_t since = lastCommandMs ? (now - lastCommandMs) / 1000 : 0;
  display.print(F("Since "));
  display.print(since / 60);
  display.print(':');
  if (since % 60 < 10) display.print('0');
  display.print(since % 60);
  display.print(F("  FW "));
  display.print(FW_VERSION_STRING);
  y += 10;

  /* Row 5: low battery warning (blinking). */
  if (batteryPercent >= 0 && batteryPercent <= BAT_LOW_PERCENT && (now / 500) % 2 == 0) {
    display.setCursor(2, y);
    display.print(F("LOW BATTERY"));
  }
}

/* Button pressed: big icon + label for BUTTON_SCREEN_MS. */
void drawButtonScreen(uint32_t now) {
  (void)now;
  display.clearDisplay();
  drawIcon(lastCommand, SCREEN_WIDTH / 2, 24);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  const char* name = commandName(lastCommand);
  display.setCursor(SCREEN_WIDTH / 2 - strlen(name) * 3, 44);
  display.print(name);
}

/* Connection lost screen with a blinking warning. */
void drawErrorScreen(uint32_t now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(14, 12); display.println(F("Connection Lost"));
  display.setCursor(14, 26); display.println(F("Retrying..."));
  if ((now / 400) % 2 == 0) {
    display.fillRect(48, 44, 32, 4, SSD1306_WHITE);
  }
}

/* Battery details screen. */
void drawBatteryScreen(uint32_t now) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(8, 4); display.println(F("Battery"));

  display.setTextSize(2);
  display.setCursor(8, 18);
  if (batteryPercent >= 0) {
    display.print(batteryPercent);
    display.print('%');
  } else {
    display.print(F("USB"));
  }
  display.setTextSize(1);

  display.setCursor(8, 40);
  if (batteryPercent >= 0) {
    display.print(F("Voltage "));
    display.print(batteryVoltage, 2);
    display.print('V');
  } else {
    display.print(F("No battery"));
  }

  if (batteryPercent >= 0 && batteryPercent <= BAT_LOW_PERCENT && (now / 500) % 2 == 0) {
    display.setCursor(8, 52);
    display.print(F("LOW BATTERY"));
  }
}

/* About screen. */
void drawAboutScreen(uint32_t now) {
  (void)now;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(8, 4);  display.println(F("Spotify Remote"));
  display.setCursor(8, 16); display.println(F("ESP32-WROOM-DA"));
  display.setCursor(8, 28); display.println(F("NRF24L01 PA+LNA"));
  display.setCursor(8, 40); display.print(F("FW ")); display.println(FW_VERSION_STRING);
  display.setCursor(8, 52); display.println(F("Firmware by You"));
}

/* Sleep screen (drawn right before deep sleep). */
void drawSleepScreen(uint32_t now) {
  (void)now;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(24, 20); display.println(F("Sleeping..."));
  display.setCursor(16, 34); display.println(F("Press Any Button"));
}

/* ------------------------------ frame router ------------------------------ */

/* Called from updateScreen(); throttles redraws to OLED_REFRESH_MS. */
void drawFrame(void) {
  static uint32_t lastDraw = 0;
  uint32_t now = millis();
  if (now - lastDraw < OLED_REFRESH_MS) return;
  lastDraw = now;

  switch (screen) {
    case SCREEN_BOOT:      drawBootScreen(now);    break;
    case SCREEN_SEARCH:    drawSearchScreen(now);  break;
    case SCREEN_CONNECTED: drawHomeScreen(now);    break;
    case SCREEN_ERROR:     drawErrorScreen(now);   break;
    case SCREEN_BTN_ANIM:  drawButtonScreen(now);  break;
    case SCREEN_BATTERY:   drawBatteryScreen(now); break;
    case SCREEN_ABOUT:     drawAboutScreen(now);   break;
    case SCREEN_SLEEP:     drawSleepScreen(now);   break;
  }
  display.display();
}
