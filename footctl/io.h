#ifndef _IO_H_INCLUDED_
#define _IO_H_INCLUDED_

#include <stdint.h>

typedef void (*BTNEventCallback)(uint8_t, uint8_t);

void BTNS_initialize(BTNEventCallback callback);
void LEDS_set_state(uint32_t led_states);
uint32_t BTNS_get_state(void);
void BTNS_cycle(void);

#endif
