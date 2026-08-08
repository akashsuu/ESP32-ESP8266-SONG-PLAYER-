/*
 * buttons.cpp - debounced button scanning with long-press and hold-repeat.
 *
 * All six buttons are active-low with internal pull-ups (INPUT_PULLUP).
 * Debounce window: BUTTON_DEBOUNCE_MS.
 *
 * Behaviour per button (see BUTTONS table):
 *  - short press            -> sends the media command immediately
 *  - long press (>1 s)      -> optional long action (battery / about screen)
 *  - hold repeat            -> volume buttons repeat while held
 *
 * The scanner never blocks: it is polled from loop() and uses millis().
 *
 * This is a .cpp translation unit: it includes buttons.h explicitly and
 * relies on globals.h for all shared state - never on .ino tab order.
 */

#include "buttons.h"
#include "battery.h"   /* readBattery() for the battery overlay */

/* ----------------------------- button table ------------------------------- */

static const ButtonDef BUTTONS[] = {
  { BTN_NEXT,     CMD_NEXT,     false, LONG_ACTION_NONE    },
  { BTN_PREVIOUS, CMD_PREVIOUS, false, LONG_ACTION_NONE    },
  { BTN_PLAY,     CMD_PLAY,     false, LONG_ACTION_ABOUT   },
  { BTN_VOL_UP,   CMD_VOL_UP,   true,  LONG_ACTION_NONE    },
  { BTN_VOL_DOWN, CMD_VOL_DOWN, true,  LONG_ACTION_NONE    },
  { BTN_MUTE,     CMD_MUTE,     false, LONG_ACTION_BATTERY },
};

#define NUM_BUTTONS (sizeof(BUTTONS) / sizeof(BUTTONS[0]))

static ButtonState states[NUM_BUTTONS];

/* -------------------------------- init ------------------------------------ */

void initButtons(void) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    states[i].candidateLevel = 1;
    states[i].pressed = false;
  }
}

/* ------------------------------ event helper ------------------------------ */

void fireCommand(uint8_t cmd) {
  txPending = true;
  pendingCommand = cmd;
  lastActivityMs = millis();
}

/* -------------------------------- scanner --------------------------------- */

void scanButtons(void) {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    ButtonState& st = states[i];
    uint32_t now = millis();
    uint8_t raw = (digitalRead(BUTTONS[i].pin) == LOW) ? 0 : 1;

    /* Debounce: only act after the level has been stable. */
    if (raw != st.candidateLevel) {
      st.candidateLevel = raw;
      st.changedSince = now;
    }
    if (now - st.changedSince < BUTTON_DEBOUNCE_MS) continue;

    /* Press edge. */
    if (st.candidateLevel == 0 && !st.pressed) {
      st.pressed = true;
      st.downSince = now;
      st.lastRepeat = now;
      st.longFired = false;
      continue;
    }

    /* Release edge: short press (if no long action took over). */
    if (st.candidateLevel == 1 && st.pressed) {
      st.pressed = false;
      if (!st.longFired) {
        fireCommand(BUTTONS[i].command);
      }
      continue;
    }

    if (!st.pressed) continue;

    /* Held: long-press action. */
    if (!st.longFired && now - st.downSince >= LONG_PRESS_MS) {
      st.longFired = true;
      lastActivityMs = now;
      if (BUTTONS[i].longAction == LONG_ACTION_BATTERY) {
        readBattery();   /* fresh reading for the battery screen */
        screen = SCREEN_BATTERY;
        screenUntilMs = now + STATUS_SCREEN_MS;
      } else if (BUTTONS[i].longAction == LONG_ACTION_ABOUT) {
        screen = SCREEN_ABOUT;
        screenUntilMs = now + STATUS_SCREEN_MS;
      }
    }

    /* Held: repeat for volume buttons. */
    if (BUTTONS[i].repeat && now - st.downSince >= REPEAT_START_MS) {
      if (now - st.lastRepeat >= REPEAT_INTERVAL_MS) {
        st.lastRepeat = now;
        fireCommand(BUTTONS[i].command);
      }
    }
  }
}
