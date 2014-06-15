#ifndef __LCD_H__
#define __LCD_H__

// source: arduino liquidcrystaldisplay library

#include <stdint.h>
#include <stdlib.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_CURSORON 0x02
#define LCD_BLINKON 0x01

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_2LINE 0x08

void lcd_cmd(uint8_t c);
void lcd_clear();
void lcd_data(uint8_t c);
void lcd_puts(char * str);
void lcd_init();
void lcd_cursor(uint8_t col, uint8_t row);
void lcd_autoscroll(uint8_t on);

#endif
