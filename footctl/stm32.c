#include "stm32.h"
#include "config.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#ifdef STM32_MOCK
#define INITIALIZE_LED_GPIO(LEDNUM)                                   \
  gpio_set_mode(GPIODEF_LED##LEDNUM##_PORT, GPIO_MODE_OUTPUT_2_MHZ,    \
                GPIO_CNF_OUTPUT_PUSHPULL, GPIODEF_LED##LEDNUM##_PIN);
#else
#define INITIALIZE_LED_GPIO(LEDNUM)                             \
  gpio_mode_setup(GPIODEF_LED##LEDNUM##_PORT, GPIO_MODE_OUTPUT, \
                  GPIO_PUPD_NONE, GPIODEF_LED##LEDNUM##_PIN);
#endif

#ifdef STM32_MOCK
#define INITIALIZE_BTN_GPIO(BTNNUM)                                            \
  gpio_set_mode(GPIODEF_BTN##BTNNUM##_PORT, GPIO_MODE_INPUT,                 \
                GPIO_CNF_INPUT_PULL_UPDOWN, GPIODEF_BTN##BTNNUM##_PIN); \
  gpio_set(GPIODEF_BTN##BTNNUM##_PORT, GPIODEF_BTN##BTNNUM##_PIN);
#else
#define INITIALIZE_BTN_GPIO(BTNNUM)                                     \
  gpio_mode_setup(GPIODEF_BTN##BTNNUM##_PORT, GPIO_MODE_INPUT,          \
                  GPIO_PUPD_PULLUP, GPIODEF_BTN##BTNNUM##_PIN);
#endif

#ifdef STM32_MOCK
#define INITIALIZE_OUTPUT_GPIO(GPIOPORT, GPIOPIN)\
  gpio_set_mode(GPIOPORT, GPIO_MODE_OUTPUT_50_MHZ,\
                GPIO_CNF_OUTPUT_PUSHPULL, GPIOPIN);
#else
#define INITIALIZE_OUTPUT_GPIO(GPIOPORT, GPIOPIN)\
  gpio_mode_setup(GPIOPORT, GPIO_MODE_OUTPUT,\
                  GPIO_PUPD_NONE, GPIOPIN);
#endif

void SYSTEM_initialize(void) {

  // clock setup
#ifdef STM32_MOCK
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
#else
  rcc_clock_setup_in_hsi_out_48mhz();
#endif
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
#ifdef STM32_MOCK
  systick_set_reload(8999);
#else
  systick_set_reload(5999);
#endif
#ifdef STM32_MOCK
      rcc_periph_clock_enable(RCC_AFIO);
#endif

  // enable clocks
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_USART2);
#ifdef GPIOA_USED
  rcc_periph_clock_enable(RCC_GPIOA);
#endif
#ifdef GPIOB_USED
  rcc_periph_clock_enable(RCC_GPIOB);
#endif
#ifdef GPIOC_USED
  rcc_periph_clock_enable(RCC_GPIOC);
#endif
#ifdef GPIOF_USED
  rcc_periph_clock_enable(RCC_GPIOF);
#endif

  // enable interrupts
  nvic_enable_irq(NVIC_USART1_IRQ);

  // setup GPIOs
  // USART 2
#ifdef STM32_MOCK
  gpio_set_mode(GPIODEF_MIDI_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIODEF_MIDI_TX_PIN);
#else
  gpio_mode_setup(GPIODEF_MIDI_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_MIDI_TX_PIN);
  gpio_set_af(GPIODEF_MIDI_TX_PORT, GPIO_AF1, GPIODEF_MIDI_TX_PIN);
#endif

  // USART 1
#ifdef STM32_MOCK
  gpio_set_mode(GPIODEF_FBV_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIODEF_FBV_TX_PIN);
  gpio_set_mode(GPIODEF_FBV_RX_PORT, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_FLOAT, GPIODEF_FBV_RX_PIN);
  gpio_primary_remap(0, AFIO_MAPR_USART1_REMAP | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON);
#else
  gpio_mode_setup(GPIODEF_FBV_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_FBV_RX_PIN);
  gpio_set_af(GPIODEF_FBV_RX_PORT, GPIO_AF1, GPIODEF_FBV_RX_PIN);
  gpio_mode_setup(GPIODEF_FBV_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_FBV_TX_PIN);
  gpio_set_af(GPIODEF_FBV_TX_PORT, GPIO_AF1, GPIODEF_FBV_TX_PIN);
#endif

  // LCD IF
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_D4_PORT, GPIODEF_LCD_D4_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_D5_PORT, GPIODEF_LCD_D5_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_D6_PORT, GPIODEF_LCD_D6_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_D7_PORT, GPIODEF_LCD_D7_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_RS_PORT, GPIODEF_LCD_RS_PIN);
  INITIALIZE_OUTPUT_GPIO(GPIODEF_LCD_RW_PORT, GPIODEF_LCD_RW_PIN);

  // LEDs
  INITIALIZE_LED_GPIO(0);
  INITIALIZE_LED_GPIO(1);
  INITIALIZE_LED_GPIO(2);
  INITIALIZE_LED_GPIO(3);
  INITIALIZE_LED_GPIO(4);
  INITIALIZE_LED_GPIO(5);
  INITIALIZE_LED_GPIO(6);
  INITIALIZE_LED_GPIO(7);
  INITIALIZE_LED_GPIO(8);

  // Buttons
  INITIALIZE_BTN_GPIO(1);
  INITIALIZE_BTN_GPIO(2);
  INITIALIZE_BTN_GPIO(3);
  INITIALIZE_BTN_GPIO(4);
  INITIALIZE_BTN_GPIO(5);
  INITIALIZE_BTN_GPIO(6);
  INITIALIZE_BTN_GPIO(7);
  INITIALIZE_BTN_GPIO(8);
  INITIALIZE_BTN_GPIO(9);

  // FBV
  usart_set_baudrate(USART1, 31250);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  USART_CR1(USART1) |= USART_CR1_RXNEIE;

  // MIDI
  usart_set_baudrate(USART2, 31250);
  usart_set_databits(USART2, 8);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  // enable tick
  systick_interrupt_enable();
  systick_counter_enable();

  usart_enable(USART1);
  usart_enable(USART2);

}
