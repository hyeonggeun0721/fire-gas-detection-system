#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

/* F_CPU(2단계 아키텍처 설계상 16MHz)를 기준으로 목표 보드레이트에 필요한
 * UBRR을 계산한다. 요구사항 명세서 6절의 기본값(9600bps)을 사용한다. */
#define UART_BAUD 9600UL
#define UART_UBRR ((F_CPU / 16UL / UART_BAUD) - 1)

static volatile char s_tx_buf[UART_TX_BUFFER_SIZE];
static volatile uint8_t s_tx_head;  /* 다음에 넣을 위치 (메인 루프만 변경) */
static volatile uint8_t s_tx_tail;  /* 다음에 보낼 위치 (ISR만 변경) */

void uart_init(void)
{
    UBRR0H = (uint8_t)(UART_UBRR >> 8);
    UBRR0L = (uint8_t)UART_UBRR;
    UCSR0B = (1 << TXEN0);                    /* 송신 활성화 (UDRE 인터럽트는 보낼 데이터가 있을 때만 켬) */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8비트 데이터, 패리티 없음, 정지비트 1 (8N1) */
    s_tx_head = 0;
    s_tx_tail = 0;
}

/* 송신 버퍼에 한 바이트를 넣는다. 버퍼가 가득 차면 기다리지 않고 버린다:
 * 메시지 일부를 잃는 것보다 메인 루프(센서 판단·버튼 반응)가 멈추는 것이 더 위험하다. */
void uart_putc(char c)
{
    uint8_t next = (uint8_t)((s_tx_head + 1) & (UART_TX_BUFFER_SIZE - 1));
    if (next == s_tx_tail)
    {
        return;  /* 버퍼 가득 참 */
    }
    s_tx_buf[s_tx_head] = c;
    s_tx_head = next;
    UCSR0B |= (1 << UDRIE0);  /* 송신 데이터 레지스터가 비면 인터럽트로 다음 바이트 전송 */
}

void uart_puts(const char *s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}

/* 송신 데이터 레지스터가 빌 때마다 한 바이트씩 보낸다. 다 보내면 인터럽트를 끈다. */
ISR(USART0_UDRE_vect)
{
    if (s_tx_tail == s_tx_head)
    {
        UCSR0B &= ~(1 << UDRIE0);
        return;
    }
    UDR0 = s_tx_buf[s_tx_tail];
    s_tx_tail = (uint8_t)((s_tx_tail + 1) & (UART_TX_BUFFER_SIZE - 1));
}
