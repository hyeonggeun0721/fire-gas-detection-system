#include "fire_logic.h"

fire_state_t fire_evaluate(fire_state_t prev_state, int16_t temp_c, uint8_t gas_pct,
                           uint8_t flame_detected, uint16_t *alarm_ticks)
{
    uint8_t temp_high = (temp_c >= TEMP_WARNING_C);
    uint8_t gas_high = (gas_pct >= GAS_ALARM_PERCENT);

    /* 대피는 래치(latch): 센서 값이 내려가도 테스트/리셋 버튼을 누르기 전까지 유지한다.
     * (시뮬레이션 검증에서 발견: 이전에는 매 틱 센서 값만 보고 상태를 새로 정해,
     *  가스가 내려가면 대피가 저절로 풀렸다.) */
    if (prev_state == FIRE_STATE_EVACUATE)
    {
        return FIRE_STATE_EVACUATE;
    }

    /* 요구사항 명세서 5절: "가스 임계값 초과 또는 불꽃 감지" 둘 중 하나만
     * 만족해도 경보로 판단한다. 한 번 경보가 되면 리셋 전까지 유지한다(래치).
     * (불꽃 감지는 인터럽트 플래그라 한 틱만 1이 되므로, 래치가 없으면 경보가
     *  5ms 만에 풀렸다 - 시나리오 D 실패 원인.) */
    if (prev_state == FIRE_STATE_ALARM || gas_high || flame_detected)
    {
        (*alarm_ticks)++;
        /* 경보가 약 3초 지속되거나 온도·가스가 동시에 초과하면 대피로 격상 */
        if (*alarm_ticks >= ALARM_TO_EVACUATE_TICKS || (temp_high && gas_high))
        {
            return FIRE_STATE_EVACUATE;
        }
        return FIRE_STATE_ALARM;
    }

    /* 주의(온도 상승)는 경보 전 단계라 래치하지 않는다: 온도가 내려가면 정상으로 복귀 */
    *alarm_ticks = 0;
    return temp_high ? FIRE_STATE_WARNING : FIRE_STATE_NORMAL;
}
