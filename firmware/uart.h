/*
 * USART0 UART 드라이버 (PC 관제실 통신)
 * 핀 배치: docs/architecture.md 참조 (TXD0=PE1)
 */
#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);

#endif
