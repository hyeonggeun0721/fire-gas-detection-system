/*
 * ADC 드라이버
 * 핀 배치: docs/architecture.md 참조 (ADC0=PF0 온도, ADC1=PF1 가스)
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);

/* channel: 0~7 (ADC0~ADC7). 10비트 변환 결과(0~1023)를 반환한다. */
uint16_t adc_read(uint8_t channel);

#endif
