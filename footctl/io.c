#include "io.h"
#include "config.h"
#include <string.h>

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

void BTNS_cycle(void) {

}

void LEDS_set_state(uint32_t led_states) {

}
