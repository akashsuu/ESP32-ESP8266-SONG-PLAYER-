/*
 * buttons.h - debounced button scanning with long-press and hold-repeat.
 *
 * Shared button types and prototypes. buttons.cpp is a separate
 * translation unit, so it includes this header explicitly.
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include "globals.h"

struct ButtonDef {
  uint8_t pin;
  uint8_t command;    /* short-press command                       */
  bool    repeat;     /* enable hold-to-repeat                     */
  uint8_t longAction; /* LONG_ACTION_NONE / _ABOUT                 */
};

struct ButtonState {
  uint8_t  candidateLevel;  /* raw level being debounced               */
  uint32_t changedSince;    /* when candidateLevel last changed        */
  bool     pressed;         /* button currently held (debounced)       */
  uint32_t downSince;       /* when the press started                  */
  uint32_t lastRepeat;      /* last repeat fire time                   */
  bool     longFired;       /* long-press action already executed      */
};

/* Configure the six pins (INPUT_PULLUP) and clear the state table. */
void initButtons(void);

/* Queue a media command for transmission (txPending / pendingCommand). */
void fireCommand(uint8_t cmd);

/* Poll the button matrix: debounce, edges, long press, hold repeat. */
void scanButtons(void);

#endif /* BUTTONS_H */
