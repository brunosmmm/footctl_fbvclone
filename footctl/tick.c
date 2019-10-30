#include "tick.h"
#ifdef VIRTUAL_HW
#include <time.h>
#include <stdio.h>
static struct timespec _VHW_initial_time;
static inline struct timespec timeDiff(struct timespec oldTime, struct timespec time) {
  if (time.tv_nsec < oldTime.tv_nsec)
    return (struct timespec){
      tv_sec : time.tv_sec - 1 - oldTime.tv_sec,
      tv_nsec : 1E9 + time.tv_nsec - oldTime.tv_nsec
    };
  else
    return (struct timespec){
      tv_sec : time.tv_sec - oldTime.tv_sec,
      tv_nsec : time.tv_nsec - oldTime.tv_nsec
    };
}
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
  struct timespec now, diff;
  unsigned long ms = 0;
  clock_gettime(CLOCK_MONOTONIC, &now);
  diff = timeDiff(_VHW_initial_time, now);
  ms = diff.tv_sec*1000 + diff.tv_nsec / 1000000;
  return (time_t)ms;
#endif
  return 0;
}
