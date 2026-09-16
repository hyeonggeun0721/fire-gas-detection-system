#include "fire_logic.h"

fire_state_t fire_evaluate(fire_state_t prev_state, int16_t temp_c, uint8_t gas_pct,
                           uint8_t flame_detected, uint16_t *alarm_ticks)
{
    (void)prev_state;

    uint8_t temp_high = (temp_c >= TEMP_WARNING_C);
    uint8_t gas_high = (gas_pct >= GAS_ALARM_PERCENT);

    fire_state_t next;

    /* 요구사항 명세서 5절: "가스 임계값 초과 또는 불꽃 감지" 둘 중 하나만
     * 만족해도 경보로 판단한다. */
    if (gas_high || flame_detected)
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
