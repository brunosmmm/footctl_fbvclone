#ifndef _FBV_H_INCLUDED_
#define _FBV_H_INCLUDED_

#include <stdint.h>

// maximum parameter payload size
#define MAX_PARAM_SIZE 18

// user readable flags
#define FBV_FLAG_ERR 0x10

typedef enum fbv_message_type_e
  {
   FBV_SET_LED = 0x04,
   FBV_SET_CH = 0x0C,
   FBV_SET_TXT = 0x10,
   FBV_ACK_PING = 0x01,
   FBV_SET_BNK1 = 0x0A,
   FBV_SET_BNK2 = 0x0B,
   FBV_TUN_STAT = 0x08,
   FBV_BTN_STAT = 0x81,
   FBV_CTL_STAT = 0x82,
   FBV_HNDSHAKE = 0x30
  } FBVMessageType;

typedef enum fbv_led_type_e
  {
   FBV_LED_TAP = 0x61,
   FBV_LED_MOD = 0x41,
   FBV_LED_DLY = 0x51,
   FBV_LED_CHA = 0x20,
   FBV_LED_CHB = 0x30,
   FBV_LED_CHC = 0x40,
   FBV_LED_CHD = 0x50,
   FBV_LED_SB1 = 0x12,
   FBV_LED_SB2 = 0x22,
   FBV_LED_SB3 = 0x32,
   FBV_LED_AMP = 0x01,
   FBV_LED_REV = 0x21,
   FBV_LED_WAH = 0x13
  } FBVLED;

typedef struct fbv_message_s {
  FBVMessageType msgType;
  uint8_t params[MAX_PARAM_SIZE];
  uint8_t paramSize;
} FBVMessage;

typedef void (*FBVMessageCallback)(FBVMessage);
typedef void (*FBVMessageSendByte)(uint8_t);

typedef struct fbv_fsm_cfg_s {
  FBVMessageCallback msgRx;
  FBVMessageSendByte msgTx;
} FBVStateMachineConfig;

uint8_t FBV_get_flags(void);
void FBV_initialize(FBVStateMachineConfig* cfg);
void FBV_recv_byte(uint8_t byte);
void FBV_recv_bytes(uint8_t* bytes, unsigned int size);
void FBV_send_msg(FBVMessage* msg);

#endif
