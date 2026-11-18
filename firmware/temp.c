#include "temp.h"

/* NTC 서미스터 분압 회로의 ADC 값 -> 섭씨 변환.
 *
 *   5V ── NTC(R25=10kΩ, B=3950) ──┬── ADC0(PF0)
 *                                 └── 10kΩ ── GND
 *
 *   NTC 저항   R(T) = R25 · exp( B · (1/T − 1/298.15) )   (T: 켈빈)
 *   ADC 값     raw  = 1023 · 10k / (10k + R(T))           (AVCC 5V 기준)
 *
 * 온도가 오르면 NTC 저항이 줄어 raw가 커진다. 지수 함수라 AVR에서 직접 풀면 부동소수점
 * 라이브러리가 필요하므로, -20~120°C를 5°C 간격으로 미리 계산한 표(tools/gen_ntc_table.py)를
 * 두고 구간 안은 직선 보간한다. 40°C 부근 기울기가 약 8카운트/°C라 보간 오차는 약 1°C 이내다 (tests/test_temp.c). */

#define NTC_T_MIN  (-20)
#define NTC_T_STEP 5
#define NTC_TABLE_LEN 29

static const uint16_t ntc_raw_table[NTC_TABLE_LEN] = {
      89,  116,  150,  189,  235,  285,  339,  396,
     454,  512,  567,  620,  669,  713,  753,  788,
     819,  846,  870,  890,  908,  923,  936,  947,
     956,  965,  972,  978,  983
};

int16_t adc_to_temp_c(uint16_t raw)
{
    if (raw <= ntc_raw_table[0])
    {
        return NTC_T_MIN;
    }
    if (raw >= ntc_raw_table[NTC_TABLE_LEN - 1])
    {
        return NTC_T_MIN + NTC_T_STEP * (NTC_TABLE_LEN - 1);
    }

    uint8_t i = 0;
    while (raw >= ntc_raw_table[i + 1])
    {
        i++;
    }
    /* ntc_raw_table[i] <= raw < ntc_raw_table[i+1] 구간에서 직선 보간 (반올림) */
    uint16_t lo = ntc_raw_table[i];
    uint16_t span = ntc_raw_table[i + 1] - lo;
    uint8_t part = (uint8_t)(((raw - lo) * NTC_T_STEP + span / 2) / span);  /* 0~5°C */
    return (int16_t)(NTC_T_MIN + NTC_T_STEP * (int16_t)i + part);
}
