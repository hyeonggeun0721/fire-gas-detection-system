#include <avr/io.h>

#include "adc.h"

void adc_init(void)
{
    /* 기준전압 선택 (REFS1:0 = 01 -> AVCC(5V)).
     * temp.c의 adc_to_temp_c()가 가정하는 기준전압(5V)과 반드시 일치해야 한다. */
    ADMUX = (1 << REFS0);

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
