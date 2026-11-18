/*
 * USART0 UART 드라이버 (PC 관제실 통신)
 * 핀 배치: docs/architecture.md 참조 (TXD0=PE1, RXD0=PE0)
 *
 * 인터럽트 방식 송신: uart_puts()는 문자열을 송신 버퍼에 넣기만 하고 바로 돌아오며,
 * 실제 전송은 UDRE(송신 데이터 레지스터 빔) 인터럽트가 한 바이트씩 처리한다.
 * (시뮬레이션 검증에서 발견: 이전의 블로킹 송신은 9600bps에서 메시지 하나에
 *  약 60ms 동안 메인 루프를 멈춰, 대피 직후 리셋 버튼 반응이 약 70ms 늦어졌다.)
 *
 * 인터럽트 방식 수신: 관제실 PC가 보낸 명령 글자를 RX 완료 인터럽트가 수신 버퍼에
 * 넣어 두고, 메인 루프가 uart_getc_nb()로 꺼내 처리한다 (양방향 통신).
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_TX_BUFFER_SIZE 128  /* 2의 거듭제곱. 경보+대피 메시지(약 120바이트)가 한 번에 들어감 */
#define UART_RX_BUFFER_SIZE 16   /* 2의 거듭제곱. 메인 루프가 5ms마다 비우므로 몇 글자면 충분 */

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);

/* 받은 글자가 있으면 1을 돌려주고 *c에 넣는다. 없으면 0 (기다리지 않음). */
uint8_t uart_getc_nb(char *c);

#endif
