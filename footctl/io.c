#include "io.h"
#include "config.h"
#include "tick.h"
#include <string.h>
#ifdef VIRTUAL_HW
#include <stdio.h>
#endif


typedef struct btn_control_s {
  BTNEventCallback evtCb;
  uint32_t buttonStates;
  uint8_t transientStates[IO_BTN_COUNT];
  tick_t lastCycle;
} BTNStateControl;

typedef struct exp_pedal_s {
  uint16_t expValues;
  tick_t lastCycle;
} EXPState;

static BTNStateControl btns;
static EXPState _exp;

void BTNS_initialize(BTNEventCallback callback) {
  btns.evtCb = callback;
  btns.buttonStates = 0;
  btns.lastCycle = 0;
  memset(btns.transientStates, 0, IO_BTN_COUNT);
}

void EXP_initialize(void) {
  _exp.expValues = 0;
  _exp.lastCycle = 0;
}

uint32_t BTNS_get_state(void) {
  return btns.buttonStates;
}

static uint32_t _read_btns(void) {
  // do something
#ifdef VIRTUAL_HW
  return 0;
#else
  unsigned int i = 0;
  uint32_t states = 0;
  for (i=0;i<CONFIG_BTN_COUNT;i++) {
    if (!gpio_get(BTN_PORTS[i], BTN_PINS[i])) {
      states |= (1<<i);
    }
  }
  return states;
#endif
}

static uint32_t _read_exp(void) {
  // do something
  return 0;
}

void BTNS_cycle(void) {
  unsigned int i = 0;
  uint32_t btn_state = 0;
  tick_t now = 0;
  now = TICK_get();
  if (now - btns.lastCycle < BTN_POLL_INTERVAL) {
    return;
  }

  btn_state = _read_btns();
  for (i=0; i<IO_BTN_COUNT;i++) {
    if ((btn_state & (1<<i)) ^ (btns.buttonStates & (1<<i))) {
      // states differ for this particular bit
      if (btns.transientStates[i] < BTN_DEBOUNCE_COUNT) {
        // increment debounce count
        btns.transientStates[i]++;
      }
      else {
        // commit state
        if (btn_state & (1<<i)) {
          btns.buttonStates |= (1<<i);
        } else {
          btns.buttonStates &= ~(1<<i);
        }
        // reset debounce counter
        btns.transientStates[i] = 0;
        // call event callback
        if (btns.evtCb) {
          (btns.evtCb)(i, (btn_state & (1<<i) ? 1: 0));
        }
      }
    }
  }
  btns.lastCycle = now;
}

void EXP_cycle(void) {
  tick_t now = 0;
  now = TICK_get();
  if (now - _exp.lastCycle < EXP_POLL_INTERVAL) {
    return;
  }
  _exp.expValues = _read_exp();
  _exp.lastCycle = now;
}

void LEDS_set_state(uint32_t led_states) {
#ifdef VIRTUAL_HW
  // printf("SET LED STATES: %x\n", led_states);
#else
  unsigned int i = 0;
  for(i=0;i<CONFIG_LED_COUNT;i++) {
    if (led_states & (1<<i)) {
      gpio_set(LED_PORTS[i], LED_PINS[i]);
    } else {
      gpio_clear(LED_PORTS[i], LED_PINS[i]);
    }
  }
#endif
}

uint16_t EXP_get_values(void) {
  return _exp.expValues;
}
