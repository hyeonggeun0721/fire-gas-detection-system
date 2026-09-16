#include "fire_logic.h"

fire_state_t fire_evaluate(fire_state_t prev_state, int16_t temp_c, uint8_t gas_pct,
                           uint8_t flame_detected, uint16_t *alarm_ticks)
{
    (void)prev_state;

    uint8_t temp_high = (temp_c >= TEMP_WARNING_C);
    uint8_t gas_high = (gas_pct >= GAS_ALARM_PERCENT);

    fire_state_t next;

    /* BUG: 요구사항 명세서 5절은 "가스 임계값 초과 또는 불꽃 감지" 둘 중 하나만
     * 만족해도 경보라고 정의하는데, 여기서는 gas_high 조건만 확인하고
     * flame_detected 조건이 빠져 있다. 불꽃이 감지돼도 온도/가스가 낮으면
     * 정상 상태로 남아있게 된다. */
    if (gas_high)
    {
        next = FIRE_STATE_ALARM;
    }
    else if (temp_high)
    {
        next = FIRE_STATE_WARNING;
    }
    else
    {
        next = FIRE_STATE_NORMAL;
    }

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
