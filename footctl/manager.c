#include "fbv.h"
#include "pod.h"
#include "config.h"
#include "manager.h"
#include "io.h"
#include "tick.h"
#include <string.h>
#ifdef VIRTUAL_HW
#include <stdio.h>
#include "virtual.h"
#else
#include <libopencm3/stm32/usart.h>
#include "lcd.h"
#endif

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
#define LED_TAP 0xff
#define LED_TUNER 0xff
#define LED_COUNT 0x4
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
#define FLAG_PGM_UPDATE_1 0x08
#define FLAG_PGM_UPDATE_2 0x10
#define FLAG_PGM_UPDATE_3 0x20
#define FLAG_TUNER_MODE 0x40
#define FLAG_FIRST_PING 0x80

// #define POD_RESPOND_PINGS
#define MAIN_LOOP_INTERVAL 1
#define PROBE_INTERVAL_MULT 300
#define BTN_HOLD_THRESH 500

#define MSG_QUEUE_LEN 8

#ifdef POD_RESPOND_PINGS
static const uint8_t FBV_PINGBACK[] = {0x00, 0x02, 0x00, 0x01, 0x01, 0x00};
#endif
static const char INITIAL_TEXT[2][16] = {"                ", "Initializing... "};

typedef struct manager_s {
  uint8_t fxState;
  uint8_t otherLedState;
  uint8_t flags;
  uint8_t actualProgram;
  char currentProgram[3];
  char currentText[16];
  uint32_t mainCycleTimer;
  uint32_t btnStates;
  uint16_t btnHoldCount[IO_BTN_COUNT];
  uint16_t expValues;
  FBVMessage msgQueue[MSG_QUEUE_LEN];
  uint8_t msgQueueWr;
  uint8_t msgQueueRd;
} Manager;

static Manager mgr;

static uint8_t _queue_len(void) {
  if (mgr.msgQueueWr > mgr.msgQueueRd) {
    return mgr.msgQueueWr - mgr.msgQueueRd;
  } else {
    return mgr.msgQueueRd - mgr.msgQueueWr;
  }
}

/* static void _queue_in(FBVMessage msg) { */
/*   mgr.msgQueue[mgr.msgQueueWr] = msg; */

/*   if (mgr.msgQueueWr == (MSG_QUEUE_LEN - 1)) { */
/*     mgr.msgQueueWr = 0; */
/*   } else { */
/*     mgr.msgQueueWr++; */
/*   } */
/* } */

static uint8_t _queue_out(FBVMessage* msg) {
  if (!msg) {
    return 0;
  }
  if (_queue_len() == 0) {
     return 0;
  }
  *msg = mgr.msgQueue[mgr.msgQueueRd];
  if (mgr.msgQueueRd == (MSG_QUEUE_LEN - 1)) {
    mgr.msgQueueRd = 0;
  } else {
    mgr.msgQueueRd++;
  }
  return 1;
}

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
  FBVMessage msg;
  // use a trick here for easier handling
  msg.msgType = cmd;
  if (paramSize == 1) {
    msg.params[0] = (uint8_t)((uint32_t)params);
    msg.paramSize = 1;
  }
  else {
    memcpy(&msg.params, params, paramSize);
    msg.paramSize = paramSize;
  }
  FBV_send_msg(&msg);
}

static void _pod_fx_set_state(uint8_t fxId, uint8_t state, uint8_t user) {
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
  if (user) {
    POD_set_fx_state(POD_FX_CONTROLS[fxId], state);
  }
}

static void _pod_fx_toggle_state(uint8_t fxId, uint8_t user) {
  if (fxId > POD_FX_COUNT) {
    return;
  }

  if (mgr.fxState & (1<<fxId)) {
    _pod_fx_set_state(fxId, 0, user);
  }
  else {
    _pod_fx_set_state(fxId, 1, user);
  }
}

static inline void _pod_activate_program(uint8_t program) {
  POD_change_program(program);
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
  if (msg.msgType == FBV_PING) {
    if (mgr.flags & FLAG_WAIT_POD) {
      // done waiting
      _fbv_msg(FBV_HNDSHAKE, 1, (uint8_t*)0x08);
      mgr.flags &= ~FLAG_WAIT_POD;
    } else {
#ifdef POD_RESPOND_PINGS
      // respond to ping
      _fbv_msg(FBV_ACK, 6, (uint8_t*)FBV_PINGBACK);
#endif
    }
    return;
  }

  // receive and commit states
  if (msg.msgType == FBV_SET_LED) {
    // LEDs govern FX states
    temp = _fbv_led_to_fx((FBVLED)(msg.params[0]));
    if (temp != POD_INVALID_FX) {
      if (_pod_fx_get_state(temp) != msg.params[1]) {
        // only emit state changes if state is actually different
        _pod_fx_set_state(temp, msg.params[1], 0);
      }
    }
    else {
      temp = _fbv_led_to_internal((FBVLED)(msg.params[0]));
      if (temp != LED_INVALID) {
        _set_led_state(temp, msg.params[1]);
      }
    }
#ifdef VIRTUAL_HW
    printf("VFBV: set LED 0x%hhx to %s\n", msg.params[0], msg.params[1] ? "ON": "OFF");
#endif
    return;
  }

  // handle text
  if (msg.msgType == FBV_SET_TXT) {
    if (strncmp((const char*)(msg.params+2), mgr.currentText, 16)) {
      memcpy(mgr.currentText, msg.params+2, 16);
      mgr.flags |= FLAG_DISPLAY_DIRTY;
#ifdef VIRTUAL_HW
      printf("VFBV: change text to '%s'\n", mgr.currentText);
#endif
    }
    return;
  }

  // handle program text: bank / channel
  if (msg.msgType == FBV_SET_CH) {
    if (msg.params[0] != mgr.currentProgram[2]) {
      mgr.currentProgram[2] = msg.params[0];
      mgr.flags |= FLAG_PGM_UPDATE_1;
#ifdef VIRTUAL_HW
      printf("VFBV: change channel to %c\n", mgr.currentProgram[2]);
#endif
    }
    return;
  }

  if (msg.msgType == FBV_SET_BNK1) {
    if (msg.params[0] != mgr.currentProgram[0]) {
      mgr.currentProgram[0] = msg.params[0];
      mgr.flags |= FLAG_PGM_UPDATE_2;
#ifdef VIRTUAL_HW
      printf("VFBV: change prg digit 1 to '%c'\n", mgr.currentProgram[0]);
#endif
    }
    return;
  }

  if (msg.msgType == FBV_SET_BNK2) {
    if (msg.params[0] != mgr.currentProgram[1]) {
      mgr.currentProgram[1] = msg.params[0];
      mgr.flags |= FLAG_PGM_UPDATE_3;
#ifdef VIRTUAL_HW
      printf("VFBV: change prg digit 2 to '%c'\n", mgr.currentProgram[1]);
#endif
    }
    return;
  }
}

static void _fbv_tx(uint8_t byte) {
#ifdef VIRTUAL_HW
  printf("FBV TX: %hhx\n", byte);
  VIRTUAL_fbv_rxbyte(byte);
#else
  usart_send_blocking(USART1, byte);
#endif
}

static void _pod_tx(uint8_t byte) {
#ifdef VIRTUAL_HW
  printf("MIDI TX: %hhx\n", byte);
  VIRTUAL_midi_rxbyte(byte);
#else
  usart_send_blocking(USART2, byte);
#endif
}

#ifndef VIRTUAL_HW
static void _lcd_redraw(void) {
  LCDContents display;

  memcpy((void *)display[0], mgr.currentProgram, 3);
  memset((void *)display[0] + 3, 0x20, LCD_COLS - 3);
  memcpy((void *)display[1], mgr.currentText, LCD_COLS);
  LCD_draw(&display);
}
#endif

void MANAGER_initialize(void) {
  PODStateMachineConfig podCfg;
  FBVStateMachineConfig fbvCfg;

  // setup
  fbvCfg.msgRx = _fbv_rx; //_queue_in;
  fbvCfg.msgTx = _fbv_tx;
  podCfg.msgTx = _pod_tx;
  podCfg.channel = POD_MIDI_CHANNEL - 1;

  // initialize
  FBV_initialize(&fbvCfg);
  POD_initialize(&podCfg);
  mgr.mainCycleTimer = 0;
  mgr.fxState = 0;
  mgr.otherLedState = 0;
  mgr.btnStates = 0;
  mgr.expValues = 0;
  mgr.msgQueueRd = 0;
  mgr.msgQueueWr = 0;
  memcpy(mgr.currentText, INITIAL_TEXT[1], 16);
  memset(mgr.currentProgram, 0x20, 3);
  memset(mgr.btnHoldCount, 0, IO_LED_COUNT);
  mgr.flags = FLAG_WAIT_POD|FLAG_FIRST_PING;
  #ifndef VIRTUAL_HW
  _lcd_redraw();
  #endif
}

static inline void _refresh_leds(void) {
  uint32_t led_states = 0;

  led_states |= mgr.otherLedState;
  led_states |= ((uint32_t)(mgr.fxState) << LED_COUNT);
  LEDS_set_state(led_states);
}

static void _btn_hold_evt(uint8_t btn) {
  switch (btn) {
  case BTN_TAP:
    if (!(mgr.flags & FLAG_TUNER_MODE)) {
      POD_enable_tuner();
    }
    break;
  }
}

static void _detect_btn_hold(void) {
  static uint32_t btn_states = 0;
  static uint32_t holding = 0;
  uint8_t i = 0;

  for (i=0; i<IO_BTN_COUNT; i++) {
    if ((btn_states & (1<<i)) ^ (mgr.btnStates & (1<<i))) {
      // state changed
      if (mgr.btnStates & (1<<i)) {
        if (mgr.btnHoldCount[i] < BTN_HOLD_THRESH) {
          mgr.btnHoldCount[i]++;
        } else {
          if (!(holding & (1<<i))) {
            // trigger hold event only once
            _btn_hold_evt(i);
            holding |= (1 << i);
          }
        }
      } else {
        // reset count
        mgr.btnHoldCount[i] = 0;
        holding &= ~(1<<i);
      }
    }
  }
  // save last
  btn_states = mgr.btnStates;
}

static inline void _detect_exp_change(void) {
  uint32_t exp_val = EXP_get_values();
  if (exp_val != mgr.expValues) {
    // emit MIDI message (will depend on configuration)
    if ((exp_val & 0x00FF) ^ (mgr.expValues & 0x00FF)) {
      // exp pedal 1 changed
      POD_change_control(EXP1_CC, (uint8_t)(exp_val & 0xFF));
    } else {
      // exp pedal 2 changed
      POD_change_control(EXP2_CC, (uint8_t)(exp_val>>8));
    }
    // update values
    mgr.expValues = exp_val;
  }
}


void MANAGER_cycle(void) {
  static uint32_t lastPing = 0;
  tick_t now = 0;
  uint32_t tmp = 0;
  FBVMessage msg;

  // throttle main cycle
  now = TICK_get();
  if ((now - mgr.mainCycleTimer) < MAIN_LOOP_INTERVAL) {
    return;
  }

  // process messages
  while (_queue_out(&msg)) {
    _fbv_rx(msg);
  }

  if (!(mgr.flags & FLAG_POD_ALIVE) || (mgr.flags & FLAG_FIRST_PING)) {
    // we just booted and haven't established that the POD is present yet,
    // so fire pings at it
    if (!lastPing) {
      _fbv_msg(FBV_PROBE, 1, (uint8_t*)0x00);
      mgr.flags &= ~FLAG_FIRST_PING;
      lastPing = PROBE_INTERVAL_MULT;
    }
    else {
      lastPing--;
    }
  }
  else {
    lastPing = 0;
  }

  // trigger program number update
  if (mgr.flags & (FLAG_PGM_UPDATE_1|FLAG_PGM_UPDATE_2|FLAG_PGM_UPDATE_3)) {
    // calculate actual program number
    tmp += (mgr.currentProgram[0] == 0x20 ? 0: 10*(mgr.currentProgram[0] - '0'));
    tmp += (mgr.currentProgram[1] - '1');
    tmp *= 4;
    tmp += (mgr.currentProgram[2] - 'A');
    mgr.actualProgram = tmp;
    // clear flags
    if (!(mgr.flags & FLAG_WAIT_POD)) {
      mgr.flags &= ~(FLAG_PGM_UPDATE_1 | FLAG_PGM_UPDATE_2 | FLAG_PGM_UPDATE_3);
      mgr.flags |= FLAG_DISPLAY_DIRTY;
    }
  }

  // refresh led states
  _refresh_leds();

  // manage button hold status
  _detect_btn_hold();

  // manage expression pedal change
  _detect_exp_change();

  // trigger display redraw
  if ((mgr.flags & FLAG_DISPLAY_DIRTY) && !(mgr.flags & FLAG_WAIT_POD)) {
    // redraw
    #ifndef VIRTUAL_HW
    _lcd_redraw();
    #endif
    mgr.flags &= ~FLAG_DISPLAY_DIRTY;
  }

  // update cycle timer
  mgr.mainCycleTimer = now;
}

// handle button events
void MANAGER_btn_event(uint8_t btn_id, uint8_t state) {
  uint8_t pgm = 0;
  // disable presses if starting
  if (mgr.flags & FLAG_WAIT_POD) {
    return;
  }

  // if in tuner mode, any button disables tuner mode
  if (mgr.flags & FLAG_TUNER_MODE) {
    if (state) {
      POD_disable_tuner();
      mgr.flags &= ~FLAG_TUNER_MODE;
    }
    return ;
  }

  // asynchronously dispatch messages
  if (state) {
    switch(btn_id) {
    case BTN_CHA:
      pgm = 4*(mgr.actualProgram / 4) + 1;
      _pod_activate_program(pgm);
      break;
    case BTN_CHB:
      pgm = 4*(mgr.actualProgram / 4) + 2;
      _pod_activate_program(pgm);
      break;
    case BTN_CHC:
      pgm = 4*(mgr.actualProgram / 4) + 3;
      _pod_activate_program(pgm);
      break;
    case BTN_CHD:
      pgm = 4*(mgr.actualProgram / 4) + 4;
      _pod_activate_program(pgm);
      break;
    case BTN_UP:
      if (pgm/4 < 15) {
        pgm = mgr.actualProgram + 4;
        _pod_activate_program(pgm);
      }
      break;
    case BTN_DN:
      if (pgm/4 > 0) {
        pgm = mgr.actualProgram - 4;
        _pod_activate_program(pgm);
      }
      break;
    case BTN_EQ:
      _pod_fx_toggle_state(POD_FX_EQ, 1);
      break;
    case BTN_MOD:
      _pod_fx_toggle_state(POD_FX_MOD, 1);
      break;
    case BTN_DLY:
      _pod_fx_toggle_state(POD_FX_DLY, 1);
      break;
    case BTN_STOMP:
      _pod_fx_toggle_state(POD_FX_STOMP, 1);
      break;
    case BTN_WAH:
      _pod_fx_toggle_state(POD_FX_WAH, 1);
      break;
    default:
      break;
    }
  } else {
    switch(btn_id) {
    case BTN_TAP:
      POD_send_tap();
      break;
    default:
      break;
    }
  }

  // update local states
  if (state) {
    mgr.btnStates |= (1<<btn_id);
  } else {
    mgr.btnStates &= ~(1<<btn_id);
  }
}
