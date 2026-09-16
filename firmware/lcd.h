/*
 * HD44780 호환 16x2 LCD 드라이버 (4비트 모드)
 * 핀 배치: docs/architecture.md 참조 (PC0=RS, PC1=E, PC2-PC5=D4-D7, RW=GND 고정)
 */
#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_string(const char *str);

#endif
