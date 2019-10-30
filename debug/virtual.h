#ifndef _VIRTUAL_H_INCLUDED_
#define _VIRTUAL_H_INCLUDED_

#include "config.h"

// Virtual Hardware

void VIRTUAL_initialize(void);
void VIRTUAL_cycle(void);
void VIRTUAL_rxbyte(uint8_t byte);

#endif
