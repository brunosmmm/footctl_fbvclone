#include "stm32.h"
#include "config.h"

#define INITIALIZE_LED_GPIO(LEDNUM)                             \
  gpio_mode_setup(GPIODEF_LED##LEDNUM##_PORT, GPIO_MODE_OUTPUT, \
                  GPIO_PUPD_NONE, GPIODEF_LED##LEDNUM##_PIN);

#define INITIALIZE_BTN_GPIO(BTNNUM)                                     \
  gpio_mode_setup(GPIODEF_BTN##BTNNUM##_PORT, GPIO_MODE_INPUT,          \
                  GPIO_PUPD_PULLUP, GPIODEF_BTN##BTNNUM##_PIN);

void SYSTEM_initialize(void) {
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

  // setup GPIOs
  // USART 2
  gpio_mode_setup(GPIODEF_MIDI_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_MIDI_TX_PIN);
  gpio_set_af(GPIODEF_MIDI_TX_PORT, GPIO_AF1, GPIODEF_MIDI_TX_PIN);

  // USART 1
  gpio_mode_setup(GPIODEF_FBV_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_FBV_RX_PIN);
  gpio_set_af(GPIODEF_FBV_RX_PORT, GPIO_AF1, GPIODEF_FBV_RX_PIN);
  gpio_mode_setup(GPIODEF_FBV_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIODEF_FBV_TX_PIN);
  gpio_set_af(GPIODEF_FBV_TX_PORT, GPIO_AF1, GPIODEF_FBV_TX_PIN);

  // LCD IF
  gpio_mode_setup(GPIODEF_LCD_D4_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_D4_PIN);
  gpio_mode_setup(GPIODEF_LCD_D5_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_D5_PIN);
  gpio_mode_setup(GPIODEF_LCD_D6_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_D6_PIN);
  gpio_mode_setup(GPIODEF_LCD_D7_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_D7_PIN);
  gpio_mode_setup(GPIODEF_LCD_E_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_E_PIN);
  gpio_mode_setup(GPIODEF_LCD_RS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIODEF_LCD_RS_PIN);

  // LEDs
  INITIALIZE_LED_GPIO(0);
  INITIALIZE_LED_GPIO(1);
  INITIALIZE_LED_GPIO(2);
  INITIALIZE_LED_GPIO(3);
  INITIALIZE_LED_GPIO(4);
  INITIALIZE_LED_GPIO(5);

  // Buttons
  INITIALIZE_BTN_GPIO(1);
  INITIALIZE_BTN_GPIO(2);
  INITIALIZE_BTN_GPIO(3);
  INITIALIZE_BTN_GPIO(4);
  INITIALIZE_BTN_GPIO(5);
  INITIALIZE_BTN_GPIO(6);
  INITIALIZE_BTN_GPIO(7);
  INITIALIZE_BTN_GPIO(8);

  // FBV
  usart_set_baudrate(USART1, 31250);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, USART_CR2_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  // MIDI
  usart_set_baudrate(USART2, 31250);
  usart_set_databits(USART2, 8);
  usart_set_parity(USART2, USART_PARITY_NONE);
  usart_set_stopbits(USART2, USART_CR2_STOPBITS_1);
  usart_set_mode(USART2, USART_MODE_TX);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

  usart_enable(USART1);
  usart_enable(USART2);
}
