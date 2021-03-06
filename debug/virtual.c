#include "virtual.h"
#include "fbv.h"
#include "tick.h"
#include <stdio.h>
#include <string.h>

#define VIRTUAL_FLAG_STARTING 0x1
#define VIRTUAL_FLAG_CONNECTED 0x2
#define VIRTUAL_FLAG_PACKET_TX 0x4
#define VIRTUAL_FLAG_PACKET_RX 0x8
#define VIRTUAL_FLAG_LOAD_INITIAL 0x10
#define VIRTUAL_STARTUP_TIME 3000
#define VIRTUAL_CYCLE_INTERVAL 10

#define VIRTUAL_RXSTATE_INITIAL 0
#define VIRTUAL_RXSTATE_LEN 1
#define VIRTUAL_RXSTATE_CMD 2
#define VIRTUAL_RXSTATE_PRM 3
#define VIRTUAL_MAX_BUFFER 128

#define VIRTUAL_MIDI_RX_CMD 0
#define VIRTUAL_MIDI_RX_CCPC 1
#define VIRTUAL_MIDI_RX_VAL 2

#define BUFFER_LEN_OFFSET 0
#define BUFFER_CMD_OFFSET 1
#define BUFFER_PRM_OFFSET 2

#define MIDI_IS_CC(byte) ((byte & 0xF0) == 0xB0)
#define MIDI_IS_PC(byte) ((byte & 0xF0) == 0xC0)

// default program is 1A
#define VIRTUAL_DEFAULT_PROGRAM 1

#define VIRTUAL_FX_EQ 0x1
#define VIRTUAL_FX_MOD 0x2
#define VIRTUAL_FX_STOMP 0x4
#define VIRTUAL_FX_DLY 0x8
#define VIRTUAL_FX_AMP 0x10
#define VIRTUAL_FX_GATE 0x20
#define VIRTUAL_FX_WAH 0x40
#define VIRTUAL_FX_COUNT 7

#define VIRTUAL_FX_EQ_IDX 0
#define VIRTUAL_FX_MOD_IDX 1
#define VIRTUAL_FX_STOMP_IDX 2
#define VIRTUAL_FX_DLY_IDX 3
#define VIRTUAL_FX_AMP_IDX 4
#define VIRTUAL_FX_GATE_IDX 5
#define VIRTUAL_FX_WAH_IDX 6

typedef struct virtual_pod_s {
  uint32_t flags;
  tick_t lastCycle;
  uint8_t fbvRxState;
  uint8_t fbvPendingBytes;
  uint8_t fbvWrPtr;
  uint8_t fbvRxBuffer[VIRTUAL_MAX_BUFFER];
  uint8_t fbvTxBuffer[VIRTUAL_MAX_BUFFER];
  uint8_t txSize;
  uint8_t midiRxState;
  uint8_t midiRxBuffer[3];
  uint8_t currentProgram;
  uint32_t fxStates;
} VirtualPOD;

typedef struct program_info_s {
  char text[16];
  uint32_t fxStates;
} ProgramInfo;

static VirtualPOD pod;
const static uint8_t pod_ping[4] = {0xF0, 0x02, 0x01, 0x00};

#define VIRTUAL_PROGRAM_COUNT 5
static const ProgramInfo programs[VIRTUAL_PROGRAM_COUNT] = {
    {"Manual          ", 0x00},
    {"Program 1       ", (VIRTUAL_FX_EQ | VIRTUAL_FX_MOD | VIRTUAL_FX_AMP)},
    {"Program 2       ", (VIRTUAL_FX_EQ | VIRTUAL_FX_DLY | VIRTUAL_FX_AMP)},
    {"Program 3       ", (VIRTUAL_FX_WAH | VIRTUAL_FX_AMP)},
    {"Program 4       ", (VIRTUAL_FX_GATE | VIRTUAL_FX_AMP | VIRTUAL_FX_STOMP | VIRTUAL_FX_EQ | VIRTUAL_FX_MOD)}};

static const FBVLED led_mapping[VIRTUAL_FX_COUNT] =
  {
   FBV_LED_SB2,
   FBV_LED_MOD,
   FBV_LED_SB1,
   FBV_LED_DLY,
   FBV_LED_AMP,
   FBV_LED_SB3,
   FBV_LED_WAH
  };

static const FBVLED ch_led_mapping[4] =
  {
   FBV_LED_CHA,
   FBV_LED_CHB,
   FBV_LED_CHC,
   FBV_LED_CHD
  };

static void _dump_packet(uint8_t *packet, uint8_t size) {
  unsigned int i = 0;
  for (i = 0; i < size; i++) {
    printf("0x%hhx%c", packet[i], (i == size - 1) ? '\n' : ' ');
  }
}

static void _fbv_tx(uint8_t byte) {
  // send bytes back
  FBV_recv_byte(byte);
}

static void _fbv_tx_many(uint8_t *bytes, uint8_t size) {
  printf("VPOD: packet sent: ");
  _dump_packet(bytes, size);
  while (size--) {
    _fbv_tx(*bytes);
    bytes++;
  }
}

static void _fbv_queue_tx(uint8_t *bytes, uint8_t size) {
  memcpy(pod.fbvTxBuffer, bytes, size);
  pod.txSize = size;
  pod.flags |= VIRTUAL_FLAG_PACKET_TX;
}

static void _load_program(uint8_t program) {
  uint8_t sendBuffer[128];
  unsigned int i = 0, count = 0;
  if (program == 0) {
    // special case
    return;
  }

  if (program > VIRTUAL_PROGRAM_COUNT-1) {
    return;
  }

  // send out text change
  sendBuffer[count++] = 0xF0;
  sendBuffer[count++] = 0x13;
  sendBuffer[count++] = 0x10;
  sendBuffer[count++] = 0x00;
  sendBuffer[count++] = 0x10;
  memcpy(sendBuffer+5, programs[program].text, 16);
  count += 16;
  // send out channel change
  sendBuffer[count++] = 0xF0;
  sendBuffer[count++] = 0x02;
  sendBuffer[count++] = 0x0C;
  sendBuffer[count++] = 'A' + (program - 1)%4;
  // send out bank change (2 commands)
  sendBuffer[count++] = 0xF0;
  sendBuffer[count++] = 0x02;
  sendBuffer[count++] = 0x0A;
  sendBuffer[count++] = (program < 37) ? ' ' : '1';
  sendBuffer[count++] = 0xF0;
  sendBuffer[count++] = 0x02;
  sendBuffer[count++] = 0x0B;
  sendBuffer[count++] = ((program-1)/4) == 9 ? '0': '1' + (program - 1)/4;
  // send out LED status (diff)

  if (program > 0) {
    sendBuffer[count++] = 0xF0;
    sendBuffer[count++] = 0x03;
    sendBuffer[count++] = 0x04;
    sendBuffer[count++] = ch_led_mapping[(program-1)%4];
    sendBuffer[count++] = 0x01;
  }

  if (!(pod.flags & VIRTUAL_FLAG_LOAD_INITIAL) && pod.currentProgram > 0) {
    // must also turn off previous channel LED
    sendBuffer[count++] = 0xF0;
    sendBuffer[count++] = 0x03;
    sendBuffer[count++] = 0x04;
    sendBuffer[count++] = ch_led_mapping[(pod.currentProgram-1)%4];
  }

  for (i = 0; i < VIRTUAL_FX_COUNT; i++) {
    if ((pod.fxStates & (1<<i)) ^ (programs[program].fxStates & (1<<i))) {
      // fx state changed
      sendBuffer[count++] = 0xF0;
      sendBuffer[count++] = 0x03;
      sendBuffer[count++] = 0x04;
      sendBuffer[count++] = led_mapping[i];
      if ((programs[program].fxStates & (1<<i))) {
        // send ON
        sendBuffer[count++] = 1;
        pod.fxStates |= (1<<i);
      } else {
        // send OFF
        sendBuffer[count++] = 0;
        pod.fxStates &= ~(1<<i);
      }
    }
  }

  _fbv_tx_many(sendBuffer, count);
  pod.currentProgram = program;
}

static void _change_fx_state(uint8_t fxId, uint8_t state, uint8_t fbv_send) {
  uint8_t sendBuffer[5];
  if (state) {
    pod.fxStates |= (1<<fxId);
  } else {
    pod.fxStates &= ~(1<<fxId);
  }
  if (fbv_send) {
    sendBuffer[0] = 0xF0; sendBuffer[1] = 0x03; sendBuffer[2] = 0x04;
    sendBuffer[3] = led_mapping[fxId]; sendBuffer[4] = state ? 1 : 0;
    _fbv_queue_tx(sendBuffer, 5);
  }
}

static void _change_control(uint8_t control, uint8_t value) {
  switch(control) {
  case BOD_CTL_MOD_EN:
    if (value > 63) {
      _change_fx_state(VIRTUAL_FX_MOD_IDX, 1, 1);
    } else {
      _change_fx_state(VIRTUAL_FX_MOD_IDX, 0, 1);
    }
    break;
  case BOD_CTL_DLY_EN:
    if (value > 63) {
      _change_fx_state(VIRTUAL_FX_DLY_IDX, 1, 1);
    } else {
      _change_fx_state(VIRTUAL_FX_DLY_IDX, 0, 1);
    }
    break;
  case BOD_CTL_STOMP_EN:
    if (value > 63) {
      _change_fx_state(VIRTUAL_FX_STOMP_IDX, 1, 1);
    } else {
      _change_fx_state(VIRTUAL_FX_STOMP_IDX, 0, 1);
    }
    break;
  case BOD_CTL_WAH_EN:
    if (value > 63) {
      _change_fx_state(VIRTUAL_FX_WAH_IDX, 1, 1);
    } else {
      _change_fx_state(VIRTUAL_FX_WAH_IDX, 0, 1);
    }
    break;
  case BOD_CTL_EQ_ENABLE:
    if (value > 63) {
      _change_fx_state(VIRTUAL_FX_EQ_IDX, 1, 1);
    } else {
      _change_fx_state(VIRTUAL_FX_EQ_IDX, 0, 1);
    }
    break;
  default:
    break;
  }
}

static void _fbv_packet_received() {
  printf("VPOD: FBV packet received: 0xF0 ");
  _dump_packet(pod.fbvRxBuffer, pod.fbvWrPtr);

  if (!(pod.flags & VIRTUAL_FLAG_CONNECTED)) {
    if (pod.fbvRxBuffer[BUFFER_CMD_OFFSET] == 0x90) {
      // ping from FBV, send back a bunch of garbage and a pingback
      pod.flags |= (VIRTUAL_FLAG_CONNECTED | VIRTUAL_FLAG_LOAD_INITIAL);
      _fbv_queue_tx((uint8_t *)pod_ping, 4);
    }
  } else {
    // normal operation
    switch (pod.fbvRxBuffer[BUFFER_CMD_OFFSET]) {
    case 0x80:
      _fbv_queue_tx((uint8_t *)pod_ping, 4);
      break;
    default:
      break;
    }
  }
}

static void _midi_packet_received()  {
  printf("VPOD: MIDI packet received: 0xF0 ");
  _dump_packet(pod.midiRxBuffer, MIDI_IS_PC(pod.midiRxBuffer[0]) ? 2 : 3);

  if (MIDI_IS_PC(pod.midiRxBuffer[0])) {
    if (pod.midiRxBuffer[1] > 127) {
      return;
    }
    _load_program(pod.midiRxBuffer[1]);
  } else {
    if (pod.midiRxBuffer[1] > 127 || pod.midiRxBuffer[2] > 127) {
      return;
    }
    _change_control(pod.midiRxBuffer[1], pod.midiRxBuffer[2]);
  }
}

void VIRTUAL_initialize(void) {
  printf("INFO: Virtual HW initialized\n");
  memset(&pod, 0, sizeof(VirtualPOD));
  pod.flags = VIRTUAL_FLAG_STARTING;
  pod.fbvRxState = VIRTUAL_RXSTATE_INITIAL;
}

void VIRTUAL_cycle(void) {
  tick_t now = TICK_get();

  if (pod.flags & VIRTUAL_FLAG_STARTING) {
    if (now > VIRTUAL_STARTUP_TIME) {
      pod.flags &= ~VIRTUAL_FLAG_STARTING;
      printf("INFO: Virtual POD available\n");
    }
  }

  if (now - pod.lastCycle < VIRTUAL_CYCLE_INTERVAL) {
    return;
  }

  if (pod.flags & VIRTUAL_FLAG_LOAD_INITIAL) {
    _load_program(VIRTUAL_DEFAULT_PROGRAM);
    pod.flags &= ~VIRTUAL_FLAG_LOAD_INITIAL;
  }

  if (pod.flags & VIRTUAL_FLAG_PACKET_TX) {
    _fbv_tx_many(pod.fbvTxBuffer, pod.txSize);
    pod.flags &= ~VIRTUAL_FLAG_PACKET_TX;
  }
  if (pod.flags & VIRTUAL_FLAG_PACKET_RX) {
    _fbv_packet_received();
    pod.flags &= ~VIRTUAL_FLAG_PACKET_RX;
  }

  pod.lastCycle = now;
}

void VIRTUAL_fbv_rxbyte(uint8_t byte) {
  if (pod.flags & VIRTUAL_FLAG_STARTING) {
    // ignore
    return;
  }

  switch(pod.fbvRxState) {
  case VIRTUAL_RXSTATE_INITIAL:
    if (byte != 0xF0) {
      break;
    }
    pod.fbvRxState = VIRTUAL_RXSTATE_LEN;
    break;
  case VIRTUAL_RXSTATE_LEN:
    pod.fbvPendingBytes = byte;
    pod.fbvRxState = VIRTUAL_RXSTATE_CMD;
    pod.fbvRxBuffer[0] = byte;
    pod.fbvWrPtr = 1;
    break;
  case VIRTUAL_RXSTATE_CMD:
    pod.fbvPendingBytes--;
    pod.fbvRxBuffer[pod.fbvWrPtr++] = byte;
    pod.fbvRxState = VIRTUAL_RXSTATE_PRM;
    break;
  case VIRTUAL_RXSTATE_PRM:
    pod.fbvPendingBytes--;
    pod.fbvRxBuffer[pod.fbvWrPtr++] = byte;
    if (!pod.fbvPendingBytes) {
      pod.fbvRxState = VIRTUAL_RXSTATE_INITIAL;
      pod.flags |= VIRTUAL_FLAG_PACKET_RX;
    }
    break;
  default:
    pod.fbvRxState = VIRTUAL_RXSTATE_INITIAL;
    break;
  }
}

void VIRTUAL_midi_rxbyte(uint8_t byte) {
  if (pod.flags & VIRTUAL_FLAG_STARTING) {
    // ignore
    return;
  }

  switch(pod.midiRxState) {
  case VIRTUAL_MIDI_RX_CMD:
    if (MIDI_IS_CC(byte) || MIDI_IS_PC(byte)) {
      pod.midiRxState = VIRTUAL_MIDI_RX_CCPC;
      pod.midiRxBuffer[0] = byte;
    }
    break;
  case VIRTUAL_MIDI_RX_CCPC:
    pod.midiRxBuffer[1] = byte;
    if (MIDI_IS_CC(pod.midiRxBuffer[0])) {
      pod.midiRxState = VIRTUAL_MIDI_RX_VAL;
    } else {
      pod.midiRxState = VIRTUAL_MIDI_RX_CMD;
      _midi_packet_received();
    }
    break;
  case VIRTUAL_MIDI_RX_VAL:
    pod.midiRxBuffer[2] = byte;
    pod.midiRxState = VIRTUAL_MIDI_RX_CMD;
    _midi_packet_received();
    break;
  default:
    pod.midiRxState = VIRTUAL_MIDI_RX_CMD;
    break;
  }
}
