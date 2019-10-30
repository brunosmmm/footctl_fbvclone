#ifndef _IO_H_INCLUDED_
#define _IO_H_INCLUDED_

#include <stdint.h>

// Button defintions (order)

#define BTN_CHA 0x0
#define BTN_CHB 0x1
#define BTN_CHC 0x2
#define BTN_CHD 0x3
#define BTN_EQ 0x4
#define BTN_STOMP 0x5
#define BTN_MOD 0x6
#define BTN_DLY 0x7
#define BTN_UP 0x8
#define BTN_DN 0x9
#define BTN_WAH 0xa
#define BTN_TAP 0xb

// Expression pedals
#define EXP_1 0x0
#define EXP_2 0x1


// Button event callbakk
typedef void (*BTNEventCallback)(uint8_t, uint8_t);

// Button functions
void BTNS_initialize(BTNEventCallback callback);
uint32_t BTNS_get_state(void);
void BTNS_cycle(void);

// LED functions
void LEDS_set_state(uint32_t led_states);

// Expression pedals
void EXP_initialize(void);
void EXP_cycle(void);
uint32_t EXP_get_values(void);

#endif
