#include "io.h"
#include "config.h"
#include "time.h"
#include <string.h>

#define BTN_POLL_INTERVAL 20
#define BTN_DEBOUNCE_COUNT 2

typedef struct btn_control_s {
  BTNEventCallback evtCb;
  uint32_t buttonStates;
  uint8_t transientStates[IO_LED_COUNT];
  tick_t lastCycle;
} BTNStateControl;

static BTNStateControl btns;

void BTNS_initialize(BTNEventCallback callback) {
  btns.evtCb = callback;
  btns.buttonStates = 0;
  memset(btns.transientStates, 0, IO_LED_COUNT);
}

uint32_t BTNS_get_state(void) {
  return btns.buttonStates;
}

static uint32_t _read_btns(void) {
  // do something
  return 0;
}

void BTNS_cycle(void) {
  unsigned int i = 0;
  uint32_t btn_state = 0;
  if (TIME_get() - btns.lastCycle < BTN_POLL_INTERVAL) {
    return;
  }

  btn_state = _read_btns();
  for (i=0; i<IO_LED_COUNT;i++) {
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
          btns.buttonStates &= (1<<i);
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
}

void LEDS_set_state(uint32_t led_states) {

}
