/*
 * buzzer.c의 논블로킹 시퀀서 호스트 단위 테스트
 * buzzer_seq_tick()은 하드웨어 레지스터에 의존하지 않는 순수 함수라, 호출
 * 횟수(=가상의 5ms 틱 경과)만으로 패턴이 올바르게 재생되는지 검증할 수 있다.
 * (14단계에서 발견한 문제: 기존 beep()은 _delay_ms()로 최대 600ms(3회 삑)
 *  동안 메인 루프를 막았다. buzzer_seq_tick()은 각 호출이 O(1)로 즉시
 *  반환되므로, 같은 패턴을 메인 루프를 막지 않고 재생할 수 있다.)
 */
#include <stdio.h>
#include <stdint.h>

#include "../buzzer.h"

int main(void)
{
    buzzer_seq_t seq;
    int failures = 0;

    for (uint8_t beep_count = 1; beep_count <= 3; beep_count++)
    {
        buzzer_seq_start(&seq, beep_count);

        int on_ticks = 0;
        int total_ticks = 0;
        int guard = 10000;  /* 무한루프 방지용 안전장치 */

        while (seq.phase != BUZZER_PHASE_IDLE && guard-- > 0)
        {
            if (buzzer_seq_tick(&seq)) on_ticks++;
            total_ticks++;
        }

        int expected_on = beep_count * BUZZER_BEEP_ON_TICKS;
        printf("beep_count=%u -> 총 %d틱 진행, ON 틱 %d개 (기대값 %d)\n",
               beep_count, total_ticks, on_ticks, expected_on);

        if (on_ticks != expected_on)
        {
            printf("FAIL: beep_count=%u에서 ON 틱 수가 예상과 다름\n", beep_count);
            failures++;
        }
        if (guard <= 0)
        {
            printf("FAIL: beep_count=%u 시퀀스가 IDLE로 끝나지 않음(무한 루프 의심)\n", beep_count);
            failures++;
        }
    }

    /* beep_count=0은 즉시 무음(IDLE)이어야 한다 */
    buzzer_seq_start(&seq, 0);
    if (seq.phase != BUZZER_PHASE_IDLE || buzzer_seq_tick(&seq) != 0)
    {
        printf("FAIL: beep_count=0인데 IDLE 상태가 아니거나 소리가 남\n");
        failures++;
    }

    if (failures > 0)
    {
        printf("FAIL: %d개 검증 실패\n", failures);
        return 1;
    }

    printf("PASS\n");
    return 0;
}
