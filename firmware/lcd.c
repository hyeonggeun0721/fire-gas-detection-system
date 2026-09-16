#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

#define LCD_DDR  DDRC
#define LCD_PORT PORTC

#define LCD_RS (1 << PC0)
#define LCD_E  (1 << PC1)
#define LCD_D4 (1 << PC2)
#define LCD_D5 (1 << PC3)
#define LCD_D6 (1 << PC4)
#define LCD_D7 (1 << PC5)
#define LCD_DATA_MASK (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)

static void lcd_pulse_enable(void)
{
    LCD_PORT |= LCD_E;
    _delay_us(1);
    LCD_PORT &= ~LCD_E;
    _delay_us(100);
}

static void lcd_write_nibble(uint8_t nibble)
{
    LCD_PORT &= ~LCD_DATA_MASK;
    if (nibble & 0x01) LCD_PORT |= LCD_D4;
    if (nibble & 0x02) LCD_PORT |= LCD_D5;
    if (nibble & 0x04) LCD_PORT |= LCD_D6;
    if (nibble & 0x08) LCD_PORT |= LCD_D7;
    lcd_pulse_enable();
}

static void lcd_write_byte(uint8_t value, uint8_t is_data)
{
    if (is_data) LCD_PORT |= LCD_RS;
    else LCD_PORT &= ~LCD_RS;

    lcd_write_nibble(value >> 4);
    lcd_write_nibble(value & 0x0F);
}

void lcd_command(uint8_t cmd)
{
    lcd_write_byte(cmd, 0);
    _delay_us(50);
}

void lcd_data(uint8_t data)
{
    lcd_write_byte(data, 1);
    _delay_us(50);
}

void lcd_init(void)
{
    LCD_DDR |= (LCD_RS | LCD_E | LCD_DATA_MASK);
    LCD_PORT &= ~(LCD_RS | LCD_E | LCD_DATA_MASK);

    _delay_ms(20);  /* 전원 안정화 대기 (HD44780 요구: VDD 상승 후 15ms 이상) */

    /* Function Set을 바로 4비트 모드로 전송.
     * (BUG: 컨트롤러가 리셋 후 8비트 인터페이스를 기본 가정하므로,
     *  0x3 -> 0x3 -> 0x3 -> 0x2 순서의 웨이크업 니블을 먼저 보내
     *  4비트 모드로 전환시키는 절차가 반드시 필요함. 이 절차 없이
     *  바로 명령을 보내면 컨트롤러가 니블을 잘못 정렬해서 해석한다.) */
    lcd_command(0x28); /* Function Set: 4-bit, 2-line, 5x8 font */
    lcd_command(0x0C); /* Display ON, cursor off, blink off */
    lcd_command(0x06); /* Entry mode: increment, no shift */
    lcd_command(0x01); /* Clear display */
    _delay_ms(2);
}

void lcd_clear(void)
{
    lcd_command(0x01);
    _delay_ms(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    addr += col;
    lcd_command(0x80 | addr);
}

void lcd_string(const char *str)
{
    while (*str)
    {
        lcd_data((uint8_t)*str++);
    }
}
