#include "virtual.h"
#include "fbv.h"
#include "tick.h"
#include <stdio.h>
#include <string.h>

#define VIRTUAL_FLAG_STARTING 0x1
#define VIRTUAL_FLAG_CONNECTED 0x2
#define VIRTUAL_FLAG_PACKET_TX 0x4
#define VIRTUAL_FLAG_PACKET_RX 0x8
#define VIRTUAL_STARTUP_TIME 3000
#define VIRTUAL_CYCLE_INTERVAL 10

#define VIRTUAL_RXSTATE_INITIAL 0
#define VIRTUAL_RXSTATE_LEN 1
#define VIRTUAL_RXSTATE_CMD 2
#define VIRTUAL_RXSTATE_PRM 3
#define VIRTUAL_MAX_BUFFER 32

#define VIRTUAL_MIDI_RX_CMD 0
#define VIRTUAL_MIDI_RX_CCPC 1
#define VIRTUAL_MIDI_RX_VAL 2

#define BUFFER_LEN_OFFSET 0
#define BUFFER_CMD_OFFSET 1
#define BUFFER_PRM_OFFSET 2

#define MIDI_IS_CC(byte) ((byte & 0xF0) == 0xB0)
#define MIDI_IS_PC(byte) ((byte & 0xF0) == 0xC0)

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
} VirtualPOD;

static VirtualPOD pod;
const static uint8_t pod_ping[4] = {0xF0, 0x02, 0x01, 0x00};

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

static void _fbv_packet_received() {
  printf("VPOD: FBV packet received: 0xF0 ");
  _dump_packet(pod.fbvRxBuffer, pod.fbvWrPtr);

  if (!(pod.flags & VIRTUAL_FLAG_CONNECTED)) {
    if (pod.fbvRxBuffer[BUFFER_CMD_OFFSET] == 0x90) {
      // ping from FBV, send back a bunch of garbage and a pingback
      pod.flags |= VIRTUAL_FLAG_CONNECTED;
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
