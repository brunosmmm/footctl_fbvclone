#include "tick.h"
#ifdef VIRTUAL_HW
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <stdio.h>
static struct timespec _VHW_initial_time;
#endif

void TICK_initialize(void) {
#ifdef VIRTUAL_HW
  printf("INFO: initializing TIME module\n");
  clock_gettime(CLOCK_MONOTONIC, &_VHW_initial_time);
#endif
}

tick_t TICK_get(void) {
  // get current time
#ifdef VIRTUAL_HW

#endif
  return 0;
}
