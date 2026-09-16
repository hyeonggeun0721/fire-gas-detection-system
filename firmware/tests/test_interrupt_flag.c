/*
 * 인터럽트 플래그 처리 패턴 호스트 단위 테스트
 * main.c의 flame_detected 플래그 처리 로직과 동일한 패턴(ISR이 플래그를
 * 세팅하고, 메인 루프가 매 틱마다 플래그를 검사해 동작을 수행)을 하드웨어
 * 없이 그대로 본떠 재현한다.
 * (9단계 버그: 플래그를 처리한 뒤 리셋하지 않으면, 물리적으로 1회 감지된
 *  이벤트가 이후 모든 루프 틱마다 반복 처리된다.)
 */
#include <stdio.h>
#include <stdint.h>

#define TOTAL_TICKS 200  /* 5ms 주기 루프 기준 약 1초 */
#define ISR_TICK    50   /* 0.25초 시점에 불꽃 감지(인터럽트 1회 발생)를 모사 */

/* main.c Week09 기능 커밋에 있던 버그: 플래그를 리셋하지 않음 */
static int action_count_buggy(void)
{
    uint8_t flag = 0;
    int count = 0;

    for (int tick = 0; tick < TOTAL_TICKS; tick++)
    {
        if (tick == ISR_TICK) flag = 1;  /* ISR 발생 모사 */

        if (flag)
        {
            count++;
            /* BUG: flag를 리셋하지 않음 */
        }
    }
    return count;
}

/* 수정 후 로직: 처리 후 flag를 리셋 */
static int action_count_fixed(void)
{
    uint8_t flag = 0;
    int count = 0;

    for (int tick = 0; tick < TOTAL_TICKS; tick++)
    {
        if (tick == ISR_TICK) flag = 1;

        if (flag)
        {
            count++;
            flag = 0;  /* 처리 후 리셋 */
        }
    }
    return count;
}

int main(void)
{
    int buggy = action_count_buggy();
    int fixed = action_count_fixed();

    printf("총 루프 틱 수: %d, 불꽃 감지(ISR)는 %d번째 틱에서 1회 발생\n", TOTAL_TICKS, ISR_TICK);
    printf("플래그 리셋 없음(버그) - 경보 처리 실행 횟수: %d\n", buggy);
    printf("플래그 리셋 있음(수정) - 경보 처리 실행 횟수: %d\n", fixed);

    if (fixed != 1)
    {
        printf("FAIL: 수정 로직에서도 물리적으로 1회인 이벤트가 %d회로 처리됨\n", fixed);
        return 1;
    }
    if (buggy <= 1)
    {
        printf("FAIL: 버그 시나리오에서 기대한 반복 트리거가 재현되지 않음\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
