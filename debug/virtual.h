#ifndef _VIRTUAL_H_INCLUDED_
#define _VIRTUAL_H_INCLUDED_

#include "config.h"

// Virtual Hardware

void VIRTUAL_initialize(void);
void VIRTUAL_cycle(void);
void VIRTUAL_fbv_rxbyte(uint8_t byte);
void VIRTUAL_midi_rxbyte(uint8_t byte);

#endif
