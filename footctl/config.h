#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include <stdint.h>
#include "ctltypes.h"

#define POD_MID_CHANNEL 1
#define IO_BTN_COUNT 12
#define IO_LED_COUNT 10

// button poll configuration
#define BTN_POLL_INTERVAL 20
#define BTN_DEBOUNCE_COUNT 2

// expression pedal poll configuration
#define EXP_POLL_INTERVAL 10
// TODO: implement variable configuration
#define EXP1_CC BOD_CTL_VOL
#define EXP2_CC BOD_CTL_WAHPOS

typedef uint32_t tick_t;

#endif
