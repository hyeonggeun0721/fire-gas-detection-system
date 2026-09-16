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

    /* HD44780은 리셋 후 8비트 인터페이스를 기본 가정하므로, 4비트 모드로
     * 전환하려면 데이터시트가 규정한 웨이크업 니블 시퀀스(0x3 x3 -> 0x2)를
     * 먼저 보내야 한다. RS=0 상태에서 상위 니블만 전송(하위 니블 없이)한다. */
    lcd_write_nibble(0x03);
    _delay_ms(5);
    lcd_write_nibble(0x03);
    _delay_us(150);
    lcd_write_nibble(0x03);
    _delay_us(150);
    lcd_write_nibble(0x02); /* 4비트 모드로 전환 */
    _delay_us(150);

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
