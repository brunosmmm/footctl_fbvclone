#include "fbv.h"
#include "pod.h"
#include "config.h"
#include "manager.h"
#include <string.h>

// setup togglable FX bits for internal state
#define POD_FX_EQ 0x0
#define POD_FX_STOMP 0x1
#define POD_FX_MOD 0x2
#define POD_FX_DLY 0x3
#define POD_FX_GATE 0x4
#define POD_FX_AMP 0x5
#define POD_FX_WAH 0x6
#define POD_FX_COUNT 0x7
#define POD_INVALID_FX 0xFF

// Channel LEDs
#define LED_CHANNEL_A 0x0
#define LED_CHANNEL_B 0x1
#define LED_CHANNEL_C 0x2
#define LED_CHANNEL_D 0x3
#define LED_TAP 0x4
#define LED_TUNER 0x5
#define LED_COUNT 0x6
#define LED_INVALID 0xFF

// build a few tables for FX state management
static const PODTogglableFX POD_FX_CONTROLS[POD_FX_COUNT] =
  {BOD_FX_EQ, BOD_FX_STOMP, BOD_FX_MOD, BOD_FX_DLYREV,
   BOD_FX_GATE, BOD_FX_AMP, BOD_FX_WAH};

// tables for LED management

// internal flags
#define FLAG_WAIT_POD 0x01
#define FLAG_POD_ALIVE 0x02
#define FLAG_DISPLAY_DIRTY 0x04

#define POD_RESPOND_PINGS
#define MAIN_LOOP_INTERVAL 100
#define PROBE_INTERVAL_MULT 3

static const uint8_t FBV_PINGBACK[] = {0x00, 0x02, 0x00, 0x01, 0x01, 0x00};

typedef struct manager_s {
  uint8_t fxState;
  uint8_t otherLedState;
  uint8_t flags;
  uint8_t currentChannel;
  char currentProgram[3];
  char currentText[16];
  uint32_t mainCycleTimer;
} Manager;

static Manager mgr;

static uint8_t _fbv_led_to_fx(FBVLED ledId) {
  switch (ledId) {
  case FBV_LED_MOD:
    return POD_FX_MOD;
  case FBV_LED_SB1:
    return POD_FX_STOMP;
  case FBV_LED_SB2:
    return POD_FX_EQ;
  case FBV_LED_SB3:
    return POD_FX_GATE;
  case FBV_LED_DLY:
    return POD_FX_DLY;
  case FBV_LED_AMP:
    return POD_FX_AMP;
  case FBV_LED_WAH:
    return POD_FX_WAH;
  default:
    return POD_INVALID_FX;
  }
}

static uint8_t _fbv_led_to_internal(FBVLED ledId) {
  switch(ledId) {
  case FBV_LED_CHA:
    return LED_CHANNEL_A;
  case FBV_LED_CHB:
    return LED_CHANNEL_B;
  case FBV_LED_CHC:
    return LED_CHANNEL_C;
  case FBV_LED_CHD:
    return LED_CHANNEL_D;
  case FBV_LED_TAP:
    return LED_TAP;
  default:
    return LED_INVALID;
  }
}

static void _fbv_msg(FBVMessageType cmd, uint8_t paramSize, uint8_t* params) {
  FBVMessage msg = {.msgType = cmd, .paramSize = paramSize};
  // use a trick here for easier handling
  if (paramSize == 1) {
    msg.params[0] = (uint8_t)((uint32_t)params);
  }
  else {
    memcpy(&msg.params, params, paramSize);
  }
  FBV_send_msg(&msg);
}

static void _pod_fx_set_state(uint8_t fxId, uint8_t state) {
  if (fxId > POD_FX_COUNT) {
    return;
  }

  // set local state
  if (state) {
    mgr.fxState |= (1<<fxId);
  }
  else {
    mgr.fxState &= ~(1<<fxId);
  }

  // emit MIDI message
  POD_set_fx_state(POD_FX_CONTROLS[fxId], state);
}

static inline uint8_t _pod_fx_get_state(uint8_t fxId) {
  if (fxId > POD_FX_COUNT) {
    return 0;
  }

  return (mgr.fxState & (1<<fxId)) ? 1 : 0;
}

static void _set_led_state(uint8_t ledId, uint8_t state) {
  if (ledId > LED_COUNT) {
    return;
  }

  if (state) {
    mgr.otherLedState |= (1<<ledId);
  } else {
    mgr.otherLedState &= ~(1<<ledId);
  }
}

// receive message from FBV
static void _fbv_rx(FBVMessage msg) {
  uint8_t temp = 0;
  // if we receive anything, then POD is alive
  mgr.flags |= FLAG_POD_ALIVE;
  if (mgr.flags & FLAG_WAIT_POD) {
    if (msg.msgType == FBV_PING) {
      // done waiting
      mgr.flags &= ~FLAG_WAIT_POD;
#ifdef POD_RESPOND_PINGS
      // respond to ping
      _fbv_msg(FBV_ACK, 6, (uint8_t*)FBV_PINGBACK);
#endif
      return;
    }
  }

  // receive and commit states
  if (msg.msgType == FBV_SET_LED) {
    // LEDs govern FX states
    temp = _fbv_led_to_fx((FBVLED)msg.params[0]);
    if (temp != POD_INVALID_FX) {
      if (_pod_fx_get_state(temp) != msg.params[1]) {
        // only emit state changes if state is actually different
        _pod_fx_set_state(temp, msg.params[1]);
      }
    }
    else {
      temp = _fbv_led_to_internal((FBVLED)msg.params[0]);
      if (temp != LED_INVALID) {
        _set_led_state(temp, msg.params[1]);
      }
    }
    return;
  }

  // handle text
  if (msg.msgType == FBV_SET_TXT) {
    if (strncmp((const char*)(msg.params+2), mgr.currentText, 16)) {
      memcpy(mgr.currentText, msg.params+2, 16);
      mgr.flags |= FLAG_DISPLAY_DIRTY;
    }
    return;
  }

  // handle program text: bank / channel
  if (msg.msgType == FBV_SET_CH) {
    if (msg.params[0] != mgr.currentProgram[2]) {
      mgr.currentProgram[2] = msg.params[0];
      mgr.currentChannel = (uint8_t)(msg.params[0] - 'A');
      mgr.flags |= FLAG_DISPLAY_DIRTY;
    }
    return;
  }

  if (msg.msgType == FBV_SET_BNK1) {
    if (msg.params[0] != mgr.currentProgram[0]) {
      mgr.currentProgram[0] = msg.params[0];
      mgr.flags |= FLAG_DISPLAY_DIRTY;
    }
    return;
  }

  if (msg.msgType == FBV_SET_BNK2) {
    if (msg.params[0] != mgr.currentProgram[1]) {
      mgr.currentProgram[1] = msg.params[0];
      mgr.flags |= FLAG_DISPLAY_DIRTY;
    }
    return;
  }
}

static void _fbv_tx(uint8_t byte) {

}

static void _pod_tx(uint8_t byte) {

}

static uint32_t _get_time(void) {
  // get current time
  return 0;
}

void MANAGER_initialize(void) {
  PODStateMachineConfig podCfg;
  FBVStateMachineConfig fbvCfg;

  // setup
  fbvCfg.msgRx = _fbv_rx;
  fbvCfg.msgTx = _fbv_tx;
  podCfg.msgTx = _pod_tx;
  podCfg.channel = POD_MID_CHANNEL;

  // initialize
  FBV_initialize(&fbvCfg);
  POD_initialize(&podCfg);
  mgr.mainCycleTimer = 0;
  mgr.fxState = 0;
  mgr.otherLedState = 0;
  memset(mgr.currentText, 0, 16);
  memset(mgr.currentProgram, 0, 3);
  mgr.flags = FLAG_WAIT_POD;
}

void MANAGER_cycle(void) {
  static uint32_t lastPing = 0;

  // throttle main cycle
  if ((_get_time() - mgr.mainCycleTimer) < MAIN_LOOP_INTERVAL) {
    return;
  }

  if (!(mgr.flags & FLAG_POD_ALIVE)) {
    // we just booted and haven't established that the POD is present yet,
    // so fire pings at it
    if (!lastPing) {
      _fbv_msg(FBV_PROBE, 1, (uint8_t*)0x00);
      lastPing = PROBE_INTERVAL_MULT;
    }
    else {
      lastPing--;
    }
  }
  else {
    lastPing = 0;
  }
}
