/*
 * remote.ino - Spotify Remote main sketch.
 *
 * ESP32-WROOM-DA + NRF24L01 PA+LNA + 0.96" SSD1306 OLED + 6 push buttons.
 *
 * This is the entry point AND the ONLY file that defines the global
 * variables (declared `extern` in globals.h). All other modules are
 * plain .cpp files with explicit .h headers, so the Arduino Web Editor
 * compiles them as separate translation units - no .ino concatenation
 * order, no auto-generated prototypes to fight with.
 *
 * Build settings (Arduino IDE / Web Editor):
 *   Board:  "ESP32 Dev Module" (espressif/arduino-esp32, core 2.0.17)
 *   Flash:  4 MB (or larger), Flash mode QIO, Upload speed 115200
 *   Libraries: RF24 (TMRh20 >= 1.4), Adafruit SSD1306, Adafruit GFX,
 *              Adafruit BusIO
 *
 * Firmware design rules honored here:
 *   - No delay() anywhere in the loop path (millis() timers only).
 *   - Single 1 KB OLED framebuffer, throttled refresh.
 *   - Static buffers, no heap allocation, no String objects.
 *   - All UI strings live in flash via F().
 */

#include "globals.h"
#include "nrf_comm.h"
#include "buttons.h"
#include "ui.h"

/* ----------------------------- hardware objects --------------------------- */
/* The only definitions of the hardware objects; globals.h declares them
 * extern so every .cpp module can use them. */

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
RF24 radio(NRF_CE_PIN, NRF_CSN_PIN);

/* -------------------------------- global state ---------------------------- */
/* DEFINITIONS live here and nowhere else (prevents multiple-definition
 * errors). The matching `extern` declarations are in globals.h. */

ScreenState screen = SCREEN_BOOT;
uint32_t    screenUntilMs    = 0;      /* when the current overlay expires    */
uint32_t    connectedSinceMs = 0;      /* when the link last came up          */

bool        linkUp           = false;  /* radio link to the receiver is alive */
bool        everConnected    = false;  /* true once a link has ever succeeded */
uint32_t    lastAckMs        = 0;      /* last radio ACK received             */
uint32_t    lastTxMs         = 0;      /* last packet sent                    */
uint32_t    lastSearchMs     = 0;      /* last CMD_CONNECT attempt            */
uint32_t    lastCommandMs    = 0;      /* uptime of the last executed command */
uint8_t     lastCommand      = CMD_NEXT;
uint8_t     linkQuality      = 0;      /* 0..100 %, TX/ACK success window     */
uint32_t    packetCounter    = 0;      /* global monotonic counter            */
uint16_t    packetNumber     = 0;      /* rolling number for duplicate detect */

bool        txPending        = false;  /* a button command awaits delivery     */
uint8_t     pendingCommand   = CMD_NEXT;

/* Local functions used by loop() below - declared explicitly so the sketch
 * never depends on the Web Editor's auto-generated prototypes. */
void updateLink(void);
void updateScreen(void);

/* ---------------------------------- setup --------------------------------- */

void setup(void) {
  delay(100); /* one-time boot settle, allowed only here */

  initButtons();
  initDisplay();
  initRadio();
  screen = SCREEN_BOOT;
  screenUntilMs = millis() + 1600;
}

/* ----------------------------------- loop --------------------------------- */

void loop(void) {
  scanButtons();
  updateLink();
  updateScreen();
}

/* --------------------------- link state machine --------------------------- */

void updateLink(void) {
  uint32_t now = millis();

  if (txPending) {
    /* A button command is queued: deliver it as soon as possible, retrying
     * every 120 ms while the link is down so the command is never dropped. */
    if (!linkUp && now - lastTxMs < 120) return;
    lastTxMs = now;
    if (sendPacket(pendingCommand)) {
      txPending = false;
      lastCommand = pendingCommand;
      lastCommandMs = now;
      screen = SCREEN_BTN_ANIM;
      screenUntilMs = now + BUTTON_SCREEN_MS;
    }
    return;
  }

  if (linkUp) {
    if (now - lastAckMs >= HEARTBEAT_INTERVAL_MS) {
      lastTxMs = now;
      sendPacket(CMD_HEARTBEAT);
    }
    if (now - lastAckMs >= LINK_TIMEOUT_MS) {
      linkUp = false;   /* radio ACKs stopped -> receiver is gone */
      if (isTransient(screen)) screenUntilMs = now;  /* drop overlays now  */
      screen = SCREEN_ERROR;                          /* show error screen */
    }
  } else {
    if (now - lastSearchMs >= SEARCH_RETRY_MS) {
      lastSearchMs = now;
      lastTxMs = now;
      sendPacket(CMD_CONNECT);
    }
  }
}

/* ------------------------------- screen manager --------------------------- */
/* isTransient() itself is implemented in ui.cpp (declared in ui.h). */

void updateScreen(void) {
  uint32_t now = millis();
  if (isTransient(screen) && now >= screenUntilMs) {
    screen = linkUp ? SCREEN_CONNECTED : (everConnected ? SCREEN_ERROR : SCREEN_SEARCH);
  }
  drawFrame();   /* defined in ui.cpp */
}
