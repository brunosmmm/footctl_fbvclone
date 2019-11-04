#include "lcd.h"
#include "tick.h"
#include <libopencm3/stm32/gpio.h>

#define LCD_CMD_CLEAR 0x01
#define LCD_CMD_HOME 0x02
#define LCD_CMD_MODE 0x04
#define LCD_CMD_DISP 0x08
#define LCD_CMD_FN 0x20
#define LCD_CMD_ADDR 0x80

#define LCD_MODE_S 0x01
#define LCD_MODE_I 0x02

#define LCD_DISP_ON 0x04
#define LCD_DISP_CUR 0x02
#define LCD_DISP_BLNK 0x01

#define LCD_FN_DL 0x10
#define LCD_FN_N 0x08
#define LCD_FN_F 0x04

#define DELAY_NS (1000000000/72000000)

const uint8_t ROWS[] = {0x00, 0x40};

inline static void _delay_ns(uint32_t amount) {
  volatile uint32_t cycles = 0;
  if (amount < DELAY_NS) {
    cycles = 1;
  } else {
    cycles = amount/DELAY_NS;
  }

  while(cycles--) {
    __asm__("nop");
  }
}
inline static void _lcd_en(void) {
  gpio_set(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
  _delay_ns(5000);
  gpio_clear(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
  _delay_ns(5000);
}

inline static void _lcd_read_mode(void) {
#ifdef STM32_MOCK
  gpio_set_mode(GPIODEF_LCD_D7_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                GPIODEF_LCD_D7_PIN);
  gpio_set_mode(GPIODEF_LCD_D6_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                GPIODEF_LCD_D6_PIN);
  gpio_set_mode(GPIODEF_LCD_D5_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                GPIODEF_LCD_D5_PIN);
  gpio_set_mode(GPIODEF_LCD_D4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                GPIODEF_LCD_D4_PIN);
#endif
  gpio_set(GPIODEF_LCD_RW_PORT, GPIODEF_LCD_RW_PIN);
  _delay_ns(500);
}

inline static void _lcd_write_mode(void) {
#ifdef STM32_MOCK
  gpio_set_mode(GPIODEF_LCD_D7_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIODEF_LCD_D7_PIN);
  gpio_set_mode(GPIODEF_LCD_D6_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIODEF_LCD_D6_PIN);
  gpio_set_mode(GPIODEF_LCD_D5_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIODEF_LCD_D5_PIN);
  gpio_set_mode(GPIODEF_LCD_D4_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIODEF_LCD_D4_PIN);
#endif
  gpio_clear(GPIODEF_LCD_RW_PORT, GPIODEF_LCD_RW_PIN);
  _delay_ns(500);
}

inline static void _lcd_wait(void) {
  uint8_t busy = 0;
  _lcd_read_mode();
  do {
    gpio_set(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
    _delay_ns(500);
    busy = gpio_get(GPIODEF_LCD_D7_PORT, GPIODEF_LCD_D7_PIN);
    gpio_clear(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
    _lcd_en();
  }
  while (busy);
  _lcd_write_mode();
}

static void _lcd_write(uint8_t data) {
  unsigned int i = 0;
  uint8_t _send = 0;
  //_lcd_wait();
  _send = data >> 4;
  for (i=0; i<4;i++) {
    if (_send & (1<<i)) {
      gpio_set(LCD_DPORTS[i], LCD_DPINS[i]);
    }
    else {
      gpio_clear(LCD_DPORTS[i], LCD_DPINS[i]);
    }
  }
  _delay_ns(200);
  _lcd_en();
  _send = data & 0x0F;
  for (i = 0; i < 4; i++) {
    if (_send & (1 << i)) {
      gpio_set(LCD_DPORTS[i], LCD_DPINS[i]);
    } else {
      gpio_clear(LCD_DPORTS[i], LCD_DPINS[i]);
    }
  }
  _delay_ns(200);
  _lcd_en();
}

static void _lcd_write_cmd(uint8_t cmd) {
  gpio_clear(GPIODEF_LCD_RS_PORT, GPIODEF_LCD_RS_PIN);
  _lcd_write(cmd);
}

static void _lcd_write_data(uint8_t data) {
  gpio_set(GPIODEF_LCD_RS_PORT, GPIODEF_LCD_RS_PIN);
  _lcd_write(data);
}

static void _lcd_cursor(uint8_t row, uint8_t col) {
  if (row > (LCD_ROWS - 1) || col >> (LCD_COLS - 1)) {
    return;
  }
  _lcd_write_cmd(LCD_CMD_ADDR + ROWS[row] + col);
}

void LCD_initialize(void) {
  _lcd_write_cmd(0x33);
  _lcd_write_cmd(0x32);
  _lcd_write_cmd(LCD_CMD_CLEAR);
  _lcd_write_cmd(LCD_CMD_DISP|LCD_DISP_ON);
  _lcd_write_cmd(LCD_CMD_MODE|LCD_MODE_I);
}

void LCD_draw(LCDContents* contents) {
  unsigned int i =0, j = 0;
  if (!contents) {
    return;
  }
  for (i=0;i<LCD_ROWS;i++) {
    _lcd_cursor(i, 0);
    for (j=0;j<LCD_COLS;j++) {
      _lcd_write_data(*((char*)contents + LCD_COLS*i + j));
    }
  }

}
