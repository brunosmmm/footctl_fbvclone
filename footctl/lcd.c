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

const uint8_t ROWS[] = {0x00, 0x40};

static void _lcd_write(uint8_t data) {
  unsigned int i = 0;
  for (i=0; i<4;i++) {
    if (data & (1<<i)) {
      gpio_set(LCD_DPORTS[i], LCD_DPINS[i]);
    }
    else {
      gpio_clear(LCD_DPORTS[i], LCD_DPINS[i]);
    }
  }
  gpio_set(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
  TICK_wait(1);
  gpio_clear(GPIODEF_LCD_E_PORT, GPIODEF_LCD_E_PIN);
}

static void _lcd_write_cmd(uint8_t cmd) {
  gpio_clear(GPIODEF_LCD_RS_PORT, GPIODEF_LCD_RS_PIN);
  _lcd_write(cmd >> 4);
  _lcd_write(cmd & 0x0F);
}

static void _lcd_write_data(uint8_t data) {
  gpio_set(GPIODEF_LCD_RS_PORT, GPIODEF_LCD_RS_PIN);
  _lcd_write(data>>4);
  _lcd_write(data&0x0F);
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
