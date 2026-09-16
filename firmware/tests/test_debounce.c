/*
 * debounce.c 호스트 단위 테스트
 * 하드웨어/시뮬레이터 없이 gcc로 직접 빌드해 디바운싱 로직만 검증한다.
 * (5단계 버그: 디바운싱 없이 원시 신호를 그대로 엣지 검출에 사용하면
 *  버튼 1회 입력이 여러 번으로 잘못 카운트되는 문제를 재현/회귀 방지)
 */
#include <stdio.h>
#include <stdint.h>

#include "../debounce.h"

/* 5ms 샘플링 주기로 관측했다고 가정한 실제 기계식 버튼의 바운스 파형
 * (1=놓임/HIGH, 0=눌림/LOW) */
static const uint8_t raw_sequence[] = {
    1, 1, 1,                            /* 유휴 상태 */
    0, 1, 0, 1, 0,                      /* 눌림 바운스 구간 (~20ms) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* 안정적으로 눌린 상태 (~50ms) */
    1, 0, 1,                            /* 릴리즈 바운스 구간 (~10ms) */
    1, 1, 1, 1, 1                       /* 유휴 상태로 복귀 */
};
#define N (sizeof(raw_sequence) / sizeof(raw_sequence[0]))

/* 5단계 초기 구현(Week05 기능 커밋)에 있던 버그: 디바운싱 없이 원시 신호로
 * 바로 엣지를 검출한다. */
static int naive_press_count(void)
{
    uint8_t last_raw = 1;
    int count = 0;
    for (unsigned i = 0; i < N; i++)
    {
        uint8_t raw = raw_sequence[i];
        if (last_raw == 1 && raw == 0) count++;
        last_raw = raw;
    }
    return count;
}

/* debounce.c를 통과한 안정 신호로 엣지를 검출한다 (수정 후 로직). */
static int debounced_press_count(void)
{
    debounce_t db;
    debounce_init(&db, 1);
    uint8_t last_stable = 1;
    int count = 0;

    for (unsigned i = 0; i < N; i++)
    {
        uint8_t stable = debounce_update(&db, raw_sequence[i]);
        if (last_stable == 1 && stable == 0) count++;
        last_stable = stable;
    }
    return count;
}

int main(void)
{
    int naive = naive_press_count();
    int debounced = debounced_press_count();

    printf("naive (디바운싱 없음) 감지된 press 횟수: %d\n", naive);
    printf("debounce.c 적용 후 감지된 press 횟수: %d\n", debounced);

    if (debounced != 1)
    {
        printf("FAIL: 물리적으로 1회 누른 입력이 %d회로 감지됨\n", debounced);
        return 1;
    }

    printf("PASS\n");
    return 0;
}
