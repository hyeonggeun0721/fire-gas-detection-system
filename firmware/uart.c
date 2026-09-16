#include <avr/io.h>

#include "uart.h"

/* BUG: F_CPU=8MHz 클럭을 가정한 예제 코드에서 UBRR 값(9600bps 기준 51)을
 * 그대로 가져왔다. 이 프로젝트는 2단계 아키텍처 설계상 F_CPU=16MHz이므로,
 * 이 값을 쓰면 실제 통신 속도가 의도한 9600bps와 크게 달라진다. */
#define UART_UBRR 51

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
