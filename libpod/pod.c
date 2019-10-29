#include "pod.h"
#include <string.h>

#define POD_FLAG_INIT 0x01

typedef struct pod_fsm_s {
  PODStateMachineConfig cfg;
  uint8_t flags;
} PODStateMachine;

static PODStateMachine fsm;

void POD_initialize(PODStateMachineConfig* cfg) {
  if (cfg) {
    fsm.cfg = *cfg;
  }
  else {
    memset(&fsm.cfg, 0, sizeof(PODStateMachineConfig));
  }
  fsm.cfg.channel &= ~0xF0;
  fsm.flags |= POD_FLAG_INIT;
}

void POD_send_msg(PODMessage* msg) {
  if (!msg) {
    return;
  }

  if (!(fsm.flags & POD_FLAG_INIT)) {
    return;
  }

  if (msg->msgType != POD_CONTROL_CHANGE && msg->msgType != POD_PROGRAM_CHANGE) {
    // invalid
    return;
  }

  if (fsm.cfg.msgTx) {
    // send first byte
    (fsm.cfg.msgTx)(msg->msgType | fsm.cfg.channel);
    if (msg->msgType == POD_CONTROL_CHANGE) {
      (fsm.cfg.msgTx)(msg->ctlType);
      (fsm.cfg.msgTx)(msg->value);
    }
    else {
      (fsm.cfg.msgTx)(msg->value);
    }
  }
}

static inline void _send_cc(PODControlType ctlType, uint8_t value) {
  PODMessage msg =
    {
     .msgType = POD_CONTROL_CHANGE,
     .ctlType = ctlType,
     .value = value
    };
  POD_send_msg(&msg);
}

// set fx state
void POD_set_fx_state(PODTogglableFX fx, uint8_t state) {
  _send_cc((PODControlType)fx, state ? 0xff : 0x00);
}

void POD_enable_tuner(void) {
  _send_cc(BOD_CTL_TUNER_EN, 0xff);
}

void POD_disable_tuner(void) {
  _send_cc(BOD_CTL_TUNER_EN, 0x00);
}
