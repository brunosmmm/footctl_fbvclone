#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include <stdint.h>
#include "ctltypes.h"
#ifndef VIRTUAL_HW
#include <libopencm3/stm32/gpio.h>
#endif

#define POD_MIDI_CHANNEL 1
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

#define CONFIG_LED_COUNT 8
#define CONFIG_BTN_COUNT 10

#ifndef VIRTUAL_HW
// mock hardware, development board
#define STM32_MOCK
#endif

#ifdef STM32_MOCK
// general definitions
#define GPIOA_USED
#define GPIOB_USED
#define GPIOC_USED
//LCD
#define GPIODEF_LCD_D4_PORT GPIOA
#define GPIODEF_LCD_D4_PIN GPIO10
#define GPIODEF_LCD_D5_PORT GPIOA
#define GPIODEF_LCD_D5_PIN GPIO11
#define GPIODEF_LCD_D6_PORT GPIOB
#define GPIODEF_LCD_D6_PIN GPIO10
#define GPIODEF_LCD_D7_PORT GPIOB
#define GPIODEF_LCD_D7_PIN GPIO11
#define GPIODEF_LCD_E_PORT GPIOA
#define GPIODEF_LCD_E_PIN GPIO9
#define GPIODEF_LCD_RS_PORT GPIOA
#define GPIODEF_LCD_RS_PIN GPIO8
//MIDI
#define GPIODEF_MIDI_TX_PORT GPIOA
#define GPIODEF_MIDI_TX_PIN GPIO2
//FBV
#define GPIODEF_FBV_TX_PORT GPIOB
#define GPIODEF_FBV_TX_PIN GPIO6
#define GPIODEF_FBV_RX_PORT GPIOB
#define GPIODEF_FBV_RX_PIN GPIO7
//LEDS
#define GPIODEF_LED0_PORT GPIOC //IO9
#define GPIODEF_LED0_PIN GPIO13
#define GPIODEF_LED1_PORT GPIOC //IO11
#define GPIODEF_LED1_PIN GPIO14
#define GPIODEF_LED2_PORT GPIOA //IO2
#define GPIODEF_LED2_PIN GPIO0
#define GPIODEF_LED3_PORT GPIOA //IO0
#define GPIODEF_LED3_PIN GPIO1
#define GPIODEF_LED4_PORT GPIOB //IO5
#define GPIODEF_LED4_PIN GPIO9
#define GPIODEF_LED5_PORT GPIOB //IO7
#define GPIODEF_LED5_PIN GPIO8
#define GPIODEF_LED6_PORT GPIOA
#define GPIODEF_LED6_PIN GPIO12
#define GPIODEF_LED7_PORT GPIOB
#define GPIODEF_LED7_PIN GPIO3
#define GPIODEF_LED8_PORT GPIOB
#define GPIODEF_LED8_PIN GPIO15

//BTNS
#define GPIODEF_BTN0_PORT GPIOB //IO8
#define GPIODEF_BTN0_PIN GPIO0
#define GPIODEF_BTN1_PORT GPIOA //IO10
#define GPIODEF_BTN1_PIN GPIO6
#define GPIODEF_BTN2_PORT GPIOA // IO3
#define GPIODEF_BTN2_PIN GPIO7
#define GPIODEF_BTN3_PORT GPIOB //IO1
#define GPIODEF_BTN3_PIN GPIO1
#define GPIODEF_BTN4_PORT GPIOA //IO4
#define GPIODEF_BTN4_PIN GPIO4
#define GPIODEF_BTN5_PORT GPIOA //IO6
#define GPIODEF_BTN5_PIN GPIO5
#define GPIODEF_BTN6_PORT GPIOA //IO8
#define GPIODEF_BTN6_PIN GPIO3
#define GPIODEF_BTN7_PORT GPIOC //IO12
#define GPIODEF_BTN7_PIN GPIO15
#define GPIODEF_BTN8_PORT GPIOB //IO13
#define GPIODEF_BTN8_PIN GPIO14
#define GPIODEF_BTN9_PORT GPIOB // IO13
#define GPIODEF_BTN9_PIN GPIO13

#else
//LCD
#define GPIODEF_LCD_D4_PORT GPIOA
#define GPIODEF_LCD_D4_PIN GPIO11
#define GPIODEF_LCD_D5_PORT GPIOA
#define GPIODEF_LCD_D5_PIN GPIO10
#define GPIODEF_LCD_D6_PORT GPIOA
#define GPIODEF_LCD_D6_PIN GPIO9
#define GPIODEF_LCD_D7_PORT GPIOA
#define GPIODEF_LCD_D7_PIN GPIO8
#define GPIODEF_LCD_E_PORT GPIOA
#define GPIODEF_LCD_E_PIN GPIO12
#define GPIODEF_LCD_RS_PORT GPIOF
#define GPIODEF_LCD_RS_PIN GPIO7
//MIDI
#define GPIODEF_MIDI_TX_PORT GPIOA
#define GPIODEF_MIDI_TX_PIN GPIO2
//FBV
#define GPIODEF_FBV_TX_PORT GPIOB
#define GPIODEF_FBV_TX_PIN GPIO6
#define GPIODEF_FBV_RX_PORT GPIOB
#define GPIODEF_FBV_RX_PIN GPIO7
//LEDS
#define GPIODEF_LED0_PORT GPIOB
#define GPIODEF_LED0_PIN GPIO14
#define GPIODEF_LED1_PORT GPIOB
#define GPIODEF_LED1_PIN GPIO12
#define GPIODEF_LED2_PORT GPIOB
#define GPIODEF_LED2_PIN GPIO10
#define GPIODEF_LED3_PORT GPIOB
#define GPIODEF_LED3_PIN GPIO1
#define GPIODEF_LED4_PORT GPIOA
#define GPIODEF_LED4_PIN GPIO7
#define GPIODEF_LED5_PORT GPIOA
#define GPIODEF_LED5_PIN GPIO5
#define GPIODEF_LED6_PORT GPIOB
#define GPIODEF_LED6_PIN GPIO4
#define GPIODEF_LED7_PORT GPIOB
#define GPIODEF_LED7_PIN GPIO8

//BTNS
#define GPIODEF_BTN0_PORT GPIOB
#define GPIODEF_BTN0_PIN GPIO15
#define GPIODEF_BTN1_PORT GPIOB
#define GPIODEF_BTN1_PIN GPIO13
#define GPIODEF_BTN2_PORT GPIOB
#define GPIODEF_BTN2_PIN GPIO11
#define GPIODEF_BTN3_PORT GPIOB
#define GPIODEF_BTN3_PIN GPIO2
#define GPIODEF_BTN4_PORT GPIOB
#define GPIODEF_BTN4_PIN GPIO0
#define GPIODEF_BTN5_PORT GPIOA
#define GPIODEF_BTN5_PIN GPIO6
#define GPIODEF_BTN6_PORT GPIOB
#define GPIODEF_BTN6_PIN GPIO3
#define GPIODEF_BTN7_PORT GPIOB
#define GPIODEF_BTN7_PIN GPIO5
#define GPIODEF_BTN8_PORT GPIOA
#define GPIODEF_BTN8_PIN GPIO4
#define GPIODEF_BTN9_PORT GPIOA
#define GPIODEF_BTN9_PIN GPIO3
#endif

extern const uint32_t LED_PORTS[];
extern const uint32_t LED_PINS[];
extern const uint32_t BTN_PORTS[];
extern const uint32_t BTN_PINS[];

#endif
