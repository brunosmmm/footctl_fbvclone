#include "fbv.h"
#include <stdio.h>
#include <stdlib.h>

// test FBV state machine

// message received callback
void message_received(FBVMessage msg) {
  printf("RX: ");
  switch (msg.msgType) {
  case FBV_SET_LED:
    printf("SET LED 0x%x to %s", msg.params[0], msg.params[1] ? "ON" : "OFF");
    break;
  case FBV_ACK_PING:
    printf("ACK/PING");
    break;
  case FBV_SET_TXT:
    printf("SET TXT to '%s'", msg.params+2);
    break;
  case FBV_SET_BNK1:
    printf("SET BNK 1 to %c", msg.params[0]);
    break;
  case FBV_SET_BNK2:
    printf("SET BNK 2 to %c", msg.params[0]);
    break;
  case FBV_SET_CH:
    printf("SET CH to %c", msg.params[0]);
    break;
  default:
    printf("CMD(%x)", msg.msgType);
    break;
   /* FBV_SET_CH = 0x0C, */
   /* FBV_SET_TXT = 0x10, */
   /* FBV_ACK_PING = 0x01, */
   /* FBV_SET_BNK1 = 0x0A, */
   /* FBV_SET_BNK2 = 0x0B, */
   /* FBV_TUN_STAT = 0x08, */
   /* FBV_BTN_STAT = 0x81, */
   /* FBV_CTL_STAT = 0x82, */
   /* FBV_HNDSHAKE = 0x30 */
  }
  printf("\n");
}

// dummy function to simulate byte sending
void dummy_send_byte(uint8_t byte) {

}

// configuration
static FBVStateMachineConfig fsmCfg =
  {
   .msgRx = message_received,
   .msgTx = dummy_send_byte
  };

// load dump
static int load_dump(const char* path) {
  FILE* fp = NULL;
  int ret = 0;
  uint8_t read = 0;

  if (!path) {
    printf("ERROR: cannot read file: %s\n", path);
    return 1;
  }

  fp = fopen(path, "rb");
  if (!fp) {
    printf("ERROR: cannot open file: %s\n", path);
    return 2;
  }

  ret = 1;
  while (ret == 1) {
    ret = fread(&read, 1, 1, fp);
    // process
    FBV_recv_byte(read);
  }

  fclose(fp);
  return 0;
}

int main(int argc, char* argv[]) {
  int ret = 0;
  // initialize
  FBV_initialize(&fsmCfg);
  if (argc < 2) {
    printf("usage: %s FILE\n", argv[0]);
    return 0;
  }

  ret = load_dump(argv[1]);
  if (ret) {
    return ret;
  }
  return 0;
}
