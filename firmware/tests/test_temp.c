/*
 * temp.c 호스트 단위 테스트 (NTC 서미스터 분압 회로)
 *
 * 1) 정확도: 서미스터 물리식으로 만든 ADC 값을 adc_to_temp_c()에 넣어
 *    0~100°C 전 구간에서 오차가 1°C 이내인지 확인한다.
 * 2) 주의 임계값(40°C) 경계: 39°C는 40 미만, 41°C는 40 이상으로 판정되는지 확인한다.
 * 3) 7단계 버그 재현: 변환표는 AVCC(5V) 기준을 가정하는데, ADMUX가 내부 2.56V 기준으로
 *    설정되어 있으면 같은 센서 전압이 더 큰 ADC 값이 되어 온도가 실제보다 크게 높게 나온다.
 */
#include <stdio.h>
#include <stdint.h>

#include "../temp.h"

/* exp(x): Makefile에 -lm 없이 빌드되도록 직접 계산 (x를 2^k로 나눠 테일러 급수 후 제곱) */
static double my_exp(double x)
{
    int k = 0;
    while (x > 0.5 || x < -0.5) { x /= 2.0; k++; }
    double term = 1.0, sum = 1.0;
    for (int n = 1; n < 20; n++) { term *= x / n; sum += term; }
    while (k--) sum *= sum;
    return sum;
}

/* 서미스터 분압 회로가 주어진 온도와 기준전압에서 만드는 ADC 값 (테스트 전용 모델) */
static uint16_t mock_adc_raw(double temp_c, double vref_v)
{
    double r_ntc = 10000.0 * my_exp(3950.0 * (1.0 / (temp_c + 273.15) - 1.0 / 298.15));
    double v = 5.0 * 10000.0 / (10000.0 + r_ntc);
    double raw = v / vref_v * 1023.0;
    if (raw > 1023.0) raw = 1023.0;
    return (uint16_t)(raw + 0.5);
}

int main(void)
{
    int fail = 0;
    int worst = 0;

    for (int t = 0; t <= 100; t++)
    {
        int16_t got = adc_to_temp_c(mock_adc_raw(t, 5.0));
        int err = got > t ? got - t : t - got;
        if (err > worst) worst = err;
        if (err > 1)
        {
            printf("FAIL: %d C -> %d C (오차 %d)\n", t, got, err);
            fail = 1;
        }
    }
    printf("0~100 C 최대 오차: %d C\n", worst);

    int16_t t39 = adc_to_temp_c(mock_adc_raw(39, 5.0));
    int16_t t41 = adc_to_temp_c(mock_adc_raw(41, 5.0));
    printf("경계: 39 C -> %d, 41 C -> %d\n", t39, t41);
    if (t39 >= 40 || t41 < 40)
    {
        printf("FAIL: 40 C 주의 임계값 경계 판정이 틀림\n");
        fail = 1;
    }

    int16_t t_ok  = adc_to_temp_c(mock_adc_raw(25, 5.0));   /* AVCC 5V 기준 (의도한 설정) */
    int16_t t_bug = adc_to_temp_c(mock_adc_raw(25, 2.56));  /* 내부 2.56V 기준 (버그 상태) */
    printf("실제 25 C: AVCC 기준 %d C, 2.56V 기준(버그) %d C\n", t_ok, t_bug);
    if (t_bug <= t_ok + 15)
    {
        printf("FAIL: 버그 시나리오에서 기대한 만큼 큰 오차가 재현되지 않음\n");
        fail = 1;
    }

    if (fail) return 1;
    printf("PASS\n");
    return 0;
}
