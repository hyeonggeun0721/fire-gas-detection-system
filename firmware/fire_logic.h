/*
 * 화재 판단 알고리즘 (상태 기계)
 * AVR 레지스터에 의존하지 않는 순수 C 로직이라 호스트 환경에서도 단위 테스트 가능.
 * 상태/임계값 정의는 docs/requirements.md 5절을 따른다.
 */
#ifndef FIRE_LOGIC_H
#define FIRE_LOGIC_H

#include <stdint.h>

typedef enum
{
    FIRE_STATE_NORMAL = 0,
    FIRE_STATE_WARNING,
    FIRE_STATE_ALARM,
    FIRE_STATE_EVACUATE,
    FIRE_STATE_COUNT
} fire_state_t;

#define TEMP_WARNING_C           40   /* 이 온도(섭씨) 이상이면 "주의" */
#define GAS_ALARM_PERCENT        50   /* 이 농도(%) 이상이면 "경보" */
#define ALARM_TO_EVACUATE_TICKS  600  /* 5ms 틱 기준 약 3초간 경보 지속 시 "대피"로 격상 */

/* 센서 값과 이전 상태로부터 다음 상태를 계산한다.
 * 경보(ALARM)와 대피(EVACUATE)는 래치된다: 한 번 들어가면 센서 값이 정상으로
 * 돌아와도 유지되고, 호출자(main.c)가 테스트/리셋 버튼으로 NORMAL로 되돌린다.
 * alarm_ticks: 경보 상태가 몇 틱째 지속되는지 누적하는 카운터. 호출자가 보관하고
 * 이 함수가 갱신한다(경보·대피가 아닐 때 0으로 리셋됨). */
fire_state_t fire_evaluate(fire_state_t prev_state, int16_t temp_c, uint8_t gas_pct,
                           uint8_t flame_detected, uint16_t *alarm_ticks);

#endif
