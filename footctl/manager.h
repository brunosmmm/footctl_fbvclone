#ifndef _MANAGER_H_INCLUDED_
#define _MANAGER_H_INCLUDED_

#include <stdint.h>

void MANAGER_initialize(void);
void MANAGER_cycle(void);
void MANAGER_btn_event(uint8_t btn_id, uint8_t state);

#endif
