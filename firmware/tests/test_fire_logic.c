/*
 * fire_logic.c 호스트 단위 테스트
 * docs/requirements.md 5절의 상태 전이 규칙(특히 "가스 임계값 초과 또는 불꽃
 * 감지 시 경보")을 하드웨어 없이 검증한다.
 * (12단계 버그: fire_evaluate()의 경보 판단 조건에 불꽃 감지가 빠져 있어,
 *  불꽃이 감지돼도 온도/가스가 낮으면 정상 상태로 남는다. 9단계에서 다뤘던
 *  "불꽃 감지가 무시되는" 문제가 통합 과정에서 다시 발생한 것이다.)
 */
#include <stdio.h>
#include <stdint.h>

#include "../fire_logic.h"

/* Week12 기능 커밋의 버그: 경보 판단에서 flame_detected 조건이 빠짐 */
static fire_state_t fire_evaluate_buggy(fire_state_t prev_state, int16_t temp_c, uint8_t gas_pct,
                                        uint8_t flame_detected, uint16_t *alarm_ticks)
{
    (void)prev_state;
    (void)flame_detected;  /* BUG: 사용하지 않음 */

    uint8_t temp_high = (temp_c >= TEMP_WARNING_C);
    uint8_t gas_high = (gas_pct >= GAS_ALARM_PERCENT);

    fire_state_t next;
    if (gas_high) next = FIRE_STATE_ALARM;
    else if (temp_high) next = FIRE_STATE_WARNING;
    else next = FIRE_STATE_NORMAL;

    if (next == FIRE_STATE_ALARM)
    {
        (*alarm_ticks)++;
        if (*alarm_ticks >= ALARM_TO_EVACUATE_TICKS || (temp_high && gas_high))
        {
            next = FIRE_STATE_EVACUATE;
        }
    }
    else
    {
        *alarm_ticks = 0;
    }
    return next;
}

static const char *state_name(fire_state_t s)
{
    switch (s)
    {
        case FIRE_STATE_NORMAL: return "NORMAL";
        case FIRE_STATE_WARNING: return "WARNING";
        case FIRE_STATE_ALARM: return "ALARM";
        case FIRE_STATE_EVACUATE: return "EVACUATE";
        default: return "?";
    }
}

int main(void)
{
    int failures = 0;

    /* 시나리오 1: 불꽃만 감지되고 온도/가스는 정상 범위. 명세상 즉시 "경보"여야 함. */
    {
        uint16_t ticks_buggy = 0, ticks_fixed = 0;
        fire_state_t buggy = fire_evaluate_buggy(FIRE_STATE_NORMAL, 25, 10, 1, &ticks_buggy);
        fire_state_t fixed = fire_evaluate(FIRE_STATE_NORMAL, 25, 10, 1, &ticks_fixed);

        printf("[시나리오1] 불꽃 감지, temp=25C, gas=10%% -> 버그: %s, 수정: %s (기대값: ALARM)\n",
               state_name(buggy), state_name(fixed));

        if (buggy == FIRE_STATE_ALARM)
        {
            printf("FAIL: 버그 로직에서 오히려 불꽃 감지가 반영됨(재현 실패)\n");
            failures++;
        }
        if (fixed != FIRE_STATE_ALARM)
        {
            printf("FAIL: 수정 로직에서 불꽃 감지가 경보로 이어지지 않음\n");
            failures++;
        }
    }

    /* 시나리오 2: 가스 단독 초과 -> 두 로직 모두 경보로 판단해야 함(회귀 없음 확인) */
    {
        uint16_t ticks = 0;
        fire_state_t s = fire_evaluate(FIRE_STATE_NORMAL, 25, 60, 0, &ticks);
        printf("[시나리오2] gas=60%% 단독 -> 수정: %s (기대값: ALARM)\n", state_name(s));
        if (s != FIRE_STATE_ALARM) { printf("FAIL\n"); failures++; }
    }

    /* 시나리오 3: 온도+가스 동시 임계값 초과 -> 즉시 EVACUATE로 격상 */
    {
        uint16_t ticks = 0;
        fire_state_t s = fire_evaluate(FIRE_STATE_NORMAL, 45, 60, 0, &ticks);
        printf("[시나리오3] temp=45C(>=40) & gas=60%%(>=50) 동시 -> 수정: %s (기대값: EVACUATE)\n",
               state_name(s));
        if (s != FIRE_STATE_EVACUATE) { printf("FAIL\n"); failures++; }
    }

    /* 시나리오 4: 경보가 ALARM_TO_EVACUATE_TICKS 만큼 지속되면 EVACUATE로 격상 */
    {
        uint16_t ticks = 0;
        fire_state_t s = FIRE_STATE_NORMAL;
        for (uint16_t i = 0; i < ALARM_TO_EVACUATE_TICKS; i++)
        {
            s = fire_evaluate(s, 25, 60, 0, &ticks);  /* gas만 계속 60% (경보 유지) */
        }
        printf("[시나리오4] gas=60%% 상태를 %u틱 지속 -> 수정: %s (기대값: EVACUATE)\n",
               ALARM_TO_EVACUATE_TICKS, state_name(s));
        if (s != FIRE_STATE_EVACUATE) { printf("FAIL\n"); failures++; }
    }

    if (failures > 0)
    {
        printf("FAIL: %d개 시나리오 실패\n", failures);
        return 1;
    }

    printf("PASS\n");
    return 0;
}
