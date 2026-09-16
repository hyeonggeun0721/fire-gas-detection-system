/*
 * buzzer.h에 선언된 순수 시퀀싱 로직 (하드웨어 레지스터에 의존하지 않음).
 * 별도 파일로 분리해 호스트 환경에서도 avr/io.h 없이 컴파일/테스트할 수 있다.
 */
#include "buzzer.h"

void buzzer_seq_start(buzzer_seq_t *seq, uint8_t beep_count)
{
    if (beep_count == 0)
    {
        seq->phase = BUZZER_PHASE_IDLE;
        seq->beeps_left = 0;
        seq->ticks_left = 0;
        return;
    }

    seq->phase = BUZZER_PHASE_ON;
    seq->beeps_left = beep_count;
    seq->ticks_left = BUZZER_BEEP_ON_TICKS;
}

uint8_t buzzer_seq_tick(buzzer_seq_t *seq)
{
    if (seq->phase == BUZZER_PHASE_IDLE)
    {
        return 0;
    }

    if (seq->ticks_left > 0)
    {
        seq->ticks_left--;
        return (seq->phase == BUZZER_PHASE_ON) ? 1 : 0;
    }

    if (seq->phase == BUZZER_PHASE_ON)
    {
        seq->beeps_left--;
        if (seq->beeps_left == 0)
        {
            seq->phase = BUZZER_PHASE_IDLE;
            return 0;
        }
        seq->phase = BUZZER_PHASE_OFF;
        seq->ticks_left = BUZZER_BEEP_OFF_TICKS;
        return 0;
    }

    /* OFF -> ON 전환. 이 호출 자체는 카운트다운을 아직 시작하지 않은
     * "준비" 단계이므로 0(꺼짐)을 반환한다. ON_TICKS만큼의 ON 신호는
     * 다음 호출부터 ticks_left 카운트다운으로 정확히 만들어진다.
     * (호스트 테스트에서 여기서 1을 반환하면 ON 구간이 매번 1틱씩 더
     * 길어지는 것을 발견해 수정했다.) */
    seq->phase = BUZZER_PHASE_ON;
    seq->ticks_left = BUZZER_BEEP_ON_TICKS;
    return 0;
}
