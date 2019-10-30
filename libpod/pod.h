#ifndef _LIBPOD_H_INCLUDED_
#define _LIBPOD_H_INCLUDED_

#include <stdint.h>
#include "ctltypes.h"

// Controls only implemented for Bass POD xt

typedef enum pod_message_type_e
  {
   POD_CONTROL_CHANGE = 0x0B,
   POD_PROGRAM_CHANGE = 0x0C
  } PODMessageType;

typedef enum pod_fx_e
  {
   BOD_FX_EQ = BOD_CTL_EQ_ENABLE,
   BOD_FX_MOD = BOD_CTL_MOD_EN,
   BOD_FX_DLYREV = BOD_CTL_DLY_EN,
   BOD_FX_STOMP = BOD_CTL_STOMP_EN,
   BOD_FX_COMP = BOD_CTL_COMP_EN,
   BOD_FX_GATE = BOD_CTL_GATE_EN,
   BOD_FX_AMP = BOD_CTL_AMP_EN,
   BOD_FX_WAH = BOD_CTL_WAH_EN
  } PODTogglableFX;

typedef struct pod_message_s {
  PODMessageType msgType;
  PODControlType ctlType;
  uint8_t value;
} PODMessage;

typedef void (*PODMessageSendByte)(uint8_t);

typedef struct pod_fsm_cfg_s {
  PODMessageSendByte msgTx;
  uint8_t channel;
} PODStateMachineConfig;


void POD_initialize(PODStateMachineConfig* cfg);
void POD_send_msg(PODMessage* msg);
void POD_set_fx_state(PODTogglableFX fx, uint8_t state);
void POD_enable_tuner(void);
void POD_disable_tuner(void);
void POD_change_program(uint8_t value);
void POD_change_control(PODControlType ctl, uint8_t value);
void POD_send_tap(void);

#endif
