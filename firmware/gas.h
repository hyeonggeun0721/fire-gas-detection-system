/*
 * ADC 원시값 -> 가스 농도(백분율) 변환
 * AVR 레지스터에 의존하지 않는 순수 C 로직
 */
#ifndef GAS_H
#define GAS_H

#include <stdint.h>

/* raw: adc_read()로 얻은 10비트 ADC 값 (0~1023). 0~100% 범위로 정규화한다. */
uint8_t adc_to_gas_percent(uint16_t raw);

#endif
