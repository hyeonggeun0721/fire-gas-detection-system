/*
 * ADC 원시값 -> 섭씨 온도 변환
 * AVR 레지스터에 의존하지 않는 순수 C 로직이라 호스트 환경에서도 단위 테스트 가능
 */
#ifndef TEMP_H
#define TEMP_H

#include <stdint.h>

/* raw: adc_read()로 얻은 10비트 ADC 값 (0~1023)
 * AVCC(5V) 기준전압, LM35 계열 아날로그 온도 센서(10mV/섭씨, 0V=0도) 가정.
 * adc.c의 ADMUX가 실제로 AVCC(5V) 기준으로 설정되어 있어야 이 공식이 맞는다. */
int16_t adc_to_temp_c(uint16_t raw);

#endif
