#include <avr/io.h>

#include "uart.h"

/* F_CPU(2단계 아키텍처 설계상 16MHz)를 기준으로 목표 보드레이트에 필요한
 * UBRR을 계산한다. 요구사항 명세서 6절의 기본값(9600bps)을 사용한다. */
#define UART_BAUD 9600UL
#define UART_UBRR ((F_CPU / 16UL / UART_BAUD) - 1)

void uart_init(void)
{
    UBRR0H = (uint8_t)(UART_UBRR >> 8);
    UBRR0L = (uint8_t)UART_UBRR;

    UCSR0B = (1 << TXEN0);                    /* 송신 활성화 */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8비트 데이터, 패리티 없음, 정지비트 1 (8N1) */
}

void uart_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
        /* 송신 버퍼가 빌 때까지 대기 */
    }
    UDR0 = c;
}

void uart_puts(const char *s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}
