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

#define BUFFER_LEN_OFFSET 0
#define BUFFER_CMD_OFFSET 1
#define BUFFER_PRM_OFFSET 2

typedef struct virtual_pod_s {
  uint32_t flags;
  tick_t lastCycle;
  uint8_t rxState;
  uint8_t pendingBytes;
  uint8_t wrPtr;
  uint8_t rxBuffer[VIRTUAL_MAX_BUFFER];
  uint8_t txBuffer[VIRTUAL_MAX_BUFFER];
  uint8_t txSize;
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
  memcpy(pod.txBuffer, bytes, size);
  pod.txSize = size;
  pod.flags |= VIRTUAL_FLAG_PACKET_TX;
}

static void _packet_received() {
  printf("VPOD: packet received: 0xF0 ");
  _dump_packet(pod.rxBuffer, pod.wrPtr);

  if (!(pod.flags & VIRTUAL_FLAG_CONNECTED)) {
    if (pod.rxBuffer[BUFFER_CMD_OFFSET] == 0x90) {
      // ping from FBV, send back a bunch of garbage and a pingback
      pod.flags |= VIRTUAL_FLAG_CONNECTED;
      _fbv_queue_tx((uint8_t *)pod_ping, 4);
    }
  } else {
    // normal operation
    switch (pod.rxBuffer[BUFFER_CMD_OFFSET]) {
    case 0x80:
      _fbv_queue_tx((uint8_t *)pod_ping, 4);
      break;
    default:
      break;
    }
  }
}

void VIRTUAL_initialize(void) {
  printf("INFO: Virtual HW initialized\n");
  pod.flags = VIRTUAL_FLAG_STARTING;
  pod.rxState = VIRTUAL_RXSTATE_INITIAL;
  pod.pendingBytes = 0;
  pod.lastCycle = 0;
  pod.txSize = 0;
  pod.wrPtr = 0;
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

  if (pod.flags & VIRTUAL_FLAG_PACKET_RX) {
    _packet_received();
    pod.flags &= ~VIRTUAL_FLAG_PACKET_RX;
  }
  if (pod.flags & VIRTUAL_FLAG_PACKET_TX) {
    _fbv_tx_many(pod.txBuffer, pod.txSize);
    pod.flags &= ~VIRTUAL_FLAG_PACKET_TX;
  }

  pod.lastCycle = now;
}



void VIRTUAL_rxbyte(uint8_t byte) {
  if (pod.flags & VIRTUAL_FLAG_STARTING) {
    // ignore
    return;
  }

  switch(pod.rxState) {
  case VIRTUAL_RXSTATE_INITIAL:
    if (byte != 0xF0) {
      break;
    }
    pod.rxState = VIRTUAL_RXSTATE_LEN;
    break;
  case VIRTUAL_RXSTATE_LEN:
    pod.pendingBytes = byte;
    pod.rxState = VIRTUAL_RXSTATE_CMD;
    pod.rxBuffer[0] = byte;
    pod.wrPtr = 1;
    break;
  case VIRTUAL_RXSTATE_CMD:
    pod.pendingBytes--;
    pod.rxBuffer[pod.wrPtr++] = byte;
    pod.rxState = VIRTUAL_RXSTATE_PRM;
    break;
  case VIRTUAL_RXSTATE_PRM:
    pod.pendingBytes--;
    pod.rxBuffer[pod.wrPtr++] = byte;
    if (!pod.pendingBytes) {
      pod.rxState = VIRTUAL_RXSTATE_INITIAL;
      pod.flags |= VIRTUAL_FLAG_PACKET_RX;
    }
    break;
  default:
    pod.rxState = VIRTUAL_RXSTATE_INITIAL;
    break;
  }
}
