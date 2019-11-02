#include "manager.h"
#include "io.h"
#include "tick.h"
#include "fbv.h"
#ifdef VIRTUAL_HW
#include <stdio.h>
#include <stdlib.h>
#include "virtual.h"
#else
#include "stm32.h"
#endif

int main(void) {

#ifdef VIRTUAL_HW
  printf("INFO: initializing\n");
  VIRTUAL_initialize();
#else
  SYSTEM_initialize();
#endif

  // initialize
  TICK_initialize();
  MANAGER_initialize();
  BTNS_initialize(MANAGER_btn_event);
  EXP_initialize();

#ifdef VIRTUAL_HW
  printf("INFO: starting main loop\n");
#endif

  for (;;) {
    BTNS_cycle();
    EXP_cycle();
    MANAGER_cycle();
#ifdef VIRTUAL_HW
    VIRTUAL_cycle();
#endif
  }
}

#ifndef VIRTUAL_HW
void usart1_isr(void) {
  static uint8_t data = 0;

  /* Check if we were called because of RXNE. */
  if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) &&
      ((USART_SR(USART1) & USART_SR_RXNE) != 0)) {

    FBV_recv_byte(usart_recv(USART1));

  }
}
#endif
