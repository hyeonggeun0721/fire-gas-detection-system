/*
 * temp.c 호스트 단위 테스트
 * ADC 기준전압 설정에 따라 동일한 센서 전압이 서로 다른 ADC 카운트로 변환되는
 * 점을 이용해, adc.c의 ADMUX 기준전압 설정 버그가 계산된 온도에 미치는 영향을
 * 하드웨어 없이 재현/검증한다.
 * (7단계 버그: 온도 변환 공식은 AVCC(5V) 기준을 가정하는데, ADMUX가 내부
 *  2.56V 기준으로 설정되어 있으면 계산된 온도가 실제보다 크게 높게 나온다.)
 */
#include <stdio.h>
#include <stdint.h>

#include "../temp.h"

/* 실제 하드웨어 ADC가 주어진 기준전압 하에서 어떤 카운트를 출력할지 계산한다.
 * (이 값은 테스트 전용 모델이며, 펌웨어 코드가 아니다.) */
static uint16_t mock_adc_raw(double sensor_mv, double vref_mv)
{
    double raw = sensor_mv / vref_mv * 1024.0;
    if (raw > 1023.0) raw = 1023.0;
    return (uint16_t)(raw + 0.5);
}

int main(void)
{
    const double sensor_mv = 250.0;  /* LM35 계열 센서, 실제 온도 25도씨 가정 (10mV/도) */

    uint16_t raw_correct = mock_adc_raw(sensor_mv, 5000.0);  /* AVCC 5V 기준 (의도한 설정) */
    uint16_t raw_buggy   = mock_adc_raw(sensor_mv, 2560.0);  /* 내부 2.56V 기준 (버그 상태) */

    int16_t temp_correct = adc_to_temp_c(raw_correct);
    int16_t temp_buggy   = adc_to_temp_c(raw_buggy);

    printf("실제 센서 온도: 25 C\n");
    printf("AVCC(5V) 기준 설정 시 계산된 온도: raw=%u -> %d C\n", raw_correct, temp_correct);
    printf("내부 2.56V 기준(버그) 설정 시 계산된 온도: raw=%u -> %d C\n", raw_buggy, temp_buggy);

    if (temp_correct < 23 || temp_correct > 27)
    {
        printf("FAIL: 올바른 기준전압 설정에서도 오차가 너무 큼\n");
        return 1;
    }
    if (temp_buggy <= temp_correct + 15)
    {
        printf("FAIL: 버그 시나리오에서 기대한 만큼 큰 오차가 재현되지 않음\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
