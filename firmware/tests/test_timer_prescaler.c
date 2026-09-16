/*
 * Timer0 CTC 프리스케일러/OCR0 계산 호스트 단위 테스트
 * main.c의 TIMER0_OCR 매크로와 동일한 계산식을 하드웨어 없이 재현해,
 * 프리스케일러 선택에 따라 8비트 OCR0 레지스터에 값이 잘려 들어가는지,
 * 그 결과 실제 주기가 목표(5ms)에서 얼마나 벗어나는지 검증한다.
 * (10단계 버그: 프리스케일러 64는 필요한 카운트 수가 255를 초과해 OCR0이
 *  잘리면서 실제 주기가 목표보다 약 5.5배 빨라진다.)
 */
#include <stdio.h>
#include <stdint.h>

#define F_CPU_HZ   16000000UL
#define TARGET_HZ  200  /* 5ms 주기 */

/* main.c의 TIMER0_OCR 매크로와 동일한 계산식 (절삭 전, 32비트) */
static long ocr_raw(uint16_t prescaler)
{
    return (long)F_CPU_HZ / prescaler / TARGET_HZ - 1;
}

/* OCR0 레지스터(8비트)에 대입될 때 실제로 저장되는 값 */
static uint8_t ocr_truncated(long raw)
{
    return (uint8_t)(raw & 0xFF);
}

/* 실제로 저장된 OCR 값과 프리스케일러로부터 역산한 실제 주기(us) */
static double actual_period_us(uint16_t prescaler, uint8_t ocr)
{
    return (double)(ocr + 1) * prescaler * 1000000.0 / F_CPU_HZ;
}

int main(void)
{
    const double target_us = 1000000.0 / TARGET_HZ;  /* 5000us */

    long raw_bug = ocr_raw(64);
    uint8_t ocr_bug = ocr_truncated(raw_bug);
    double period_bug = actual_period_us(64, ocr_bug);

    long raw_fix = ocr_raw(1024);
    uint8_t ocr_fix = ocr_truncated(raw_fix);
    double period_fix = actual_period_us(1024, ocr_fix);

    printf("목표 주기: %.0f us\n", target_us);
    printf("[프리스케일러 64]  필요 OCR0=%ld -> 8비트 저장값=%u -> 실제 주기=%.1f us\n",
           raw_bug, ocr_bug, period_bug);
    printf("[프리스케일러 1024] 필요 OCR0=%ld -> 8비트 저장값=%u -> 실제 주기=%.1f us\n",
           raw_fix, ocr_fix, period_fix);

    if (raw_bug <= 255)
    {
        printf("FAIL: 버그 시나리오에서 OCR0 초과 상황이 재현되지 않음\n");
        return 1;
    }

    double error_fix_pct = (period_fix - target_us) / target_us * 100.0;
    if (error_fix_pct < -2.0 || error_fix_pct > 2.0)
    {
        printf("FAIL: 수정 후에도 목표 주기 대비 오차가 2%%를 초과함 (%.2f%%)\n", error_fix_pct);
        return 1;
    }

    printf("PASS\n");
    return 0;
}
