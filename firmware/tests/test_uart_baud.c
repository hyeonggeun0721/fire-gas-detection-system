/*
 * UART UBRR/보드레이트 계산 호스트 단위 테스트
 * uart.c의 UBRR 설정이 실제로 만들어내는 보드레이트를 하드웨어 없이
 * 역산해, 목표 보드레이트(9600bps) 대비 오차가 통신 가능한 범위(통상 2~3%
 * 이내) 안에 드는지 검증한다.
 * (13단계 버그: F_CPU=8MHz를 가정한 예제의 UBRR 값을 실제 F_CPU=16MHz
 *  프로젝트에 그대로 사용하면, 실제 보드레이트가 목표의 약 2배가 된다.)
 */
#include <stdio.h>
#include <stdint.h>

#define F_CPU_HZ     16000000UL
#define TARGET_BAUD  9600UL

/* Week13 기능 커밋의 버그: F_CPU=8MHz 예제에서 가져온 UBRR 값을 그대로 사용 */
#define UBRR_BUGGY   51

/* main.c의 uart.c에 적용할 올바른 계산식 */
static long ubrr_fixed(void)
{
    return (long)(F_CPU_HZ / 16UL / TARGET_BAUD) - 1;
}

static double actual_baud(long ubrr)
{
    return (double)F_CPU_HZ / (16.0 * (ubrr + 1));
}

int main(void)
{
    double baud_buggy = actual_baud(UBRR_BUGGY);
    long ubrr_fix = ubrr_fixed();
    double baud_fix = actual_baud(ubrr_fix);

    double err_buggy_pct = (baud_buggy - TARGET_BAUD) / TARGET_BAUD * 100.0;
    double err_fix_pct = (baud_fix - TARGET_BAUD) / TARGET_BAUD * 100.0;

    printf("목표 보드레이트: %lu bps (F_CPU=%lu Hz)\n", TARGET_BAUD, F_CPU_HZ);
    printf("[버그] UBRR=%d  -> 실제 %.1f bps (오차 %.1f%%)\n", UBRR_BUGGY, baud_buggy, err_buggy_pct);
    printf("[수정] UBRR=%ld -> 실제 %.1f bps (오차 %.2f%%)\n", ubrr_fix, baud_fix, err_fix_pct);

    /* 일반적으로 UART는 송수신 측 보드레이트 오차가 약 2~3%를 넘으면
     * 비트 샘플링 시점이 어긋나 프레이밍 오류(통신 깨짐)가 발생하기 쉽다. */
    if (err_fix_pct < -2.0 || err_fix_pct > 2.0)
    {
        printf("FAIL: 수정 후에도 오차가 2%%를 초과함\n");
        return 1;
    }
    if (err_buggy_pct > -20.0 && err_buggy_pct < 20.0)
    {
        printf("FAIL: 버그 시나리오에서 기대한 큰 오차가 재현되지 않음\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
