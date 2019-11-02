#ifndef _LCD_H_INCLUDED_
#define _LCD_H_INCLUDED_

#define LCD_ROWS 2
#define LCD_COLS 16

typedef char LCDContents[LCD_ROWS][LCD_COLS];

void LCD_initialize(void);
void LCD_draw(LCDContents* contents);

#endif
