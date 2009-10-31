#ifndef LCD_H
#define LCD_H

void lcd_init(void);

void wcommand(unsigned char c);

void wdata(unsigned char c);

void goLine(char line);

void goClearLine(char line);

void print(const char *s);

void printInt(signed int i);

void lcd_init_seq(void);

#endif
