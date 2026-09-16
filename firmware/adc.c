#include <avr/io.h>

#include "adc.h"

void adc_init(void)
{
    /* 기준전압 선택 (REFS1:0).
     * BUG: 내부 2.56V 기준(REFS1:0 = 11)을 선택했지만, 실제 온도 변환 공식
     * (temp.c의 adc_to_temp_c)은 AVCC(5V) 기준(REFS1:0 = 01)으로 계산되도록
     * 작성되어 있다. 기준전압이 서로 다르면 동일한 센서 전압에서도 ADC
     * 카운트 값이 달라지므로 변환된 온도값이 크게 틀어진다. */
    ADMUX = (1 << REFS1) | (1 << REFS0);

    /* 분주비 128 (16MHz / 128 = 125kHz, 10비트 변환 권장 범위인
     * 50kHz~200kHz 이내이므로 이 값 자체는 올바르다) */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
    {
        /* 변환 완료 대기 */
    }

    return ADC;
}
