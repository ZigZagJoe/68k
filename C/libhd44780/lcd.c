#include <lcd.h>
#include <io.h>


//lcd is connected to QB output of the 74LS164, giving a delay of about 1.75uS
//lcd E is the inverted version of its CS output from 74138

void lcd_init() {
	uint8_t init = LCD_FUNCTIONSET | LCD_8BITMODE | LCD_2LINE;
	
	lcd_cmd(init);
	DELAY_MS(30);
	lcd_cmd(init);
	DELAY_MS(30);
	lcd_cmd(init);
	DELAY_MS(30);
	
	lcd_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
	lcd_clear();
	
	lcd_cmd(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
}

void lcd_cursor(uint8_t col, uint8_t row) {
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  lcd_cmd(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_cmd(uint8_t c) {
	MEM(IO_DEV2) = c;
	DELAY(50);
}

void lcd_data(uint8_t c) {
	MEM(IO_DEV2+2) = c;
	DELAY(50);
}

void lcd_clear() {
	lcd_cmd(LCD_CLEARDISPLAY);
	DELAY_MS(10);
}

void lcd_puts(char * str) {
	lcd_clear();
	uint8_t i=0;
	char c;
	while(*str) {
		c = *str++;
		
		if (c == '\n') {
			lcd_cursor(0,1);
			i = 16;
			continue;
		}
		
		lcd_data(c);
		
		if (i++ == 15) 
			lcd_cursor(0,1);
	}
}

void lcd_puts_raw(char * str) {
	while(*str) {
		lcd_data(*str++);
	}
}

void lcd_autoscroll(uint8_t on) {
	lcd_cmd(LCD_ENTRYMODESET | LCD_ENTRYLEFT | (on?LCD_ENTRYSHIFTINCREMENT:0));
}
