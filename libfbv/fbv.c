#include "fbv.h"
#include <string.h>

// possible states
#define FBV_STATE_RX_HDR 0
#define FBV_STATE_RX_LEN 1
#define FBV_STATE_RX_CMD 2
#define FBV_STATE_RX_PRM 3

// internal flags
#define FBV_FLAG_INIT 0x01

// user flag mask
#define FBV_USR_FLAG_MASK 0xF0

// other
#define RX_BUFFER_SIZE MAX_PARAM_SIZE + 2

typedef struct fbv_state_machine_s {
  uint8_t state;
  uint8_t flags;
  uint8_t rxBuffer[RX_BUFFER_SIZE];
  uint8_t wrPtr;
  uint8_t pendingBytes;
  FBVStateMachineConfig cfg;
} FBVStateMachine;


static FBVStateMachine fsm;

// done receiving packet
static void fbv_rx_done(void) {
  FBVMessage msg = {0};
  fsm.wrPtr = 0;

  // save message
  msg.paramSize = fsm.rxBuffer[0] - 1;
  msg.msgType = fsm.rxBuffer[1];
  memcpy(msg.params, fsm.rxBuffer+2, fsm.rxBuffer[0] - 1);

  if (msg.msgType == FBV_SET_TXT) {
    // null terminated string
    msg.params[msg.paramSize+1] = 0;
  }

  // callback with received message
  if (fsm.cfg.msgRx) {
    (fsm.cfg.msgRx)(msg);
  }
}

// get flags
uint8_t FBV_get_flags(void) {
  uint8_t flags = fsm.flags;
  fsm.flags &= ~FBV_USR_FLAG_MASK;
  return flags & FBV_USR_FLAG_MASK;
}

// Initialize state machine
void FBV_initialize(FBVStateMachineConfig* cfg) {
  fsm.state = FBV_STATE_RX_HDR;
  if (cfg) {
    fsm.cfg = *cfg;
  }
  else {
    memset(&fsm.cfg, 0, sizeof(FBVStateMachineConfig));
  }
  fsm.wrPtr = 0;
  fsm.pendingBytes = 0;
  fsm.flags = FBV_FLAG_INIT;
}

// receive byte and parse
void FBV_recv_byte(uint8_t byte) {
  if (!(fsm.flags & FBV_FLAG_INIT)){
    // not initialized
    return;
  }

  switch (fsm.state) {
  case FBV_STATE_RX_HDR:
    if (byte != 0xF0) {
      break;
    }
    fsm.state = FBV_STATE_RX_LEN;
    fsm.wrPtr = 0;
    break;
  case FBV_STATE_RX_LEN:
    if (byte > MAX_PARAM_SIZE) {
      byte = MAX_PARAM_SIZE;
    }
    fsm.pendingBytes = byte;
    fsm.rxBuffer[fsm.wrPtr++] = byte;
    fsm.state = FBV_STATE_RX_CMD;
    break;
  case FBV_STATE_RX_CMD:
    fsm.rxBuffer[fsm.wrPtr++] = byte;
    fsm.pendingBytes--;
    fsm.state = FBV_STATE_RX_PRM;
    break;
  case FBV_STATE_RX_PRM:
    fsm.rxBuffer[fsm.wrPtr++] = byte;
    fsm.pendingBytes--;
    if (!fsm.pendingBytes) {
      fsm.state = FBV_STATE_RX_HDR;
      fbv_rx_done();
      break;
    }
    break;
  default:
    fsm.pendingBytes = 0;
    fsm.wrPtr = 0;
    fsm.state = FBV_STATE_RX_HDR;
    fsm.flags |= FBV_FLAG_ERR;
    break;
  }

}

// receive multiple bytes
void FBV_recv_bytes(uint8_t* bytes, unsigned int size) {
  while (size) {
    FBV_recv_byte(*bytes);
    bytes++;
    size--;
  }
}

// send message
void FBV_send_msg(FBVMessage* msg) {
  unsigned int paramSize = 0;
  if (!msg) {
    return;
  }

  if (!(fsm.flags & FBV_FLAG_INIT)) {
    return;
  }

  // just call the send byte function if available
  if (fsm.cfg.msgTx) {
    // send header
    (fsm.cfg.msgTx)(0xF0);
    // send length
    (fsm.cfg.msgTx)(msg->paramSize + 1);
    // send command
    (fsm.cfg.msgTx)(msg->msgType);
    // send params
    paramSize = msg->paramSize;
    while (paramSize) {
      (fsm.cfg.msgTx)(msg->params[msg->paramSize-paramSize]);
      paramSize--;
    }
  }
}
