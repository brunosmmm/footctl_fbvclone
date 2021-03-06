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
#include "lcd.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#endif

// static uint8_t debugBuffer[128];
// static uint8_t bufWrPtr = 0;
#ifdef STM32_MOCK
#define USART_STATUS_REG USART_SR
#define USART_RX_ISR USART_SR_RXNE
#define USART_TX_ISR USART_SR_TXE
#else
#define USART_STATUS_REG USART_ISR
#define USART_RX_ISR USART_ISR_RXNE
#define USART_TX_ISR USART_ISR_TXE
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
#ifndef VIRTUAL_HW
  LCD_initialize();
#endif
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
  uint8_t data = 0;
  /* Check if we were called because of RXNE. */
  if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) &&
      ((USART_STATUS_REG(USART1) & USART_RX_ISR) != 0)) {

    data = usart_recv(USART1);
    FBV_recv_byte(data);
  }
  if (((USART_CR1(USART1) & USART_CR1_TXEIE) != 0) &&
      ((USART_STATUS_REG(USART1) & USART_TX_ISR) != 0)) {
    USART_CR1(USART1) &= ~USART_CR1_TXEIE;
  }
}
#endif
