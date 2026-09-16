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

/* 마지막으로 변환에 사용한 채널. 0xFF는 "아직 한 번도 읽지 않음"을 의미한다. */
static uint8_t s_last_channel = 0xFF;

static uint16_t adc_convert_once(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC))
    {
        /* 변환 완료 대기 */
    }
    return ADC;
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);

    if (channel != s_last_channel)
    {
        /* 채널을 막 전환하면 샘플-홀드 커패시터가 이전 채널의 전압에서
         * 새 채널의 전압으로 아직 완전히 안정되지 않은 상태일 수 있다.
         * 전환 직후 첫 변환 결과는 버리고, 그 다음 변환 결과만 신뢰한다. */
        adc_convert_once();
        s_last_channel = channel;
    }

    return adc_convert_once();
}
