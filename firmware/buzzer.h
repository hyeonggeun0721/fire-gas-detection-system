/*
 * 논블로킹 부저 시퀀서
 * 지정한 횟수만큼 짧게 삑삑거리는 패턴을, _delay_ms()로 메인 루프를 막지 않고
 * 매 5ms 틱마다 한 걸음씩 진행시키는 방식으로 재생한다.
 *
 * 14단계 통합 테스트에서 발견한 문제: 기존에는 beep()이 _delay_ms()로 부저를
 * 직접 울렸는데, 대피 상태 진입 시 beep(3) 호출이 (100+100)ms x 3 = 600ms
 * 동안 메인 루프를 완전히 멈춰 세웠다. 그동안 온도/가스 재평가, 테스트 버튼
 * 반응이 모두 지연되어, 화재가 급속히 악화되는 상황에서 안전에 영향을 줄 수
 * 있는 지연이었다. 이 모듈은 그 문제를 해결하기 위해 도입했다.
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#define BUZZER_BEEP_ON_TICKS  20  /* 5ms 틱 기준 100ms */
#define BUZZER_BEEP_OFF_TICKS 20  /* 5ms 틱 기준 100ms */

typedef enum
{
    BUZZER_PHASE_IDLE = 0,
    BUZZER_PHASE_ON,
    BUZZER_PHASE_OFF
} buzzer_phase_t;

typedef struct
{
    buzzer_phase_t phase;
    uint8_t beeps_left;
    uint16_t ticks_left;
} buzzer_seq_t;

/* seq를 beep_count번 삑거리는 패턴으로 (재)시작한다. */
void buzzer_seq_start(buzzer_seq_t *seq, uint8_t beep_count);

/* 한 틱(5ms)만큼 시퀀스를 진행시키고, 이번 틱에 부저를 켜야 하면 1, 꺼야
 * 하면 0을 반환한다. 하드웨어 레지스터에 의존하지 않는 순수 함수라
 * 호스트 환경에서도 단위 테스트 가능하다. */
uint8_t buzzer_seq_tick(buzzer_seq_t *seq);

void buzzer_init(void);
void buzzer_start(uint8_t beep_count);
/* 재생 중인 패턴을 즉시 멈추고 부저를 끈다 (리셋 시 사용) */
void buzzer_stop(void);
void buzzer_tick(void);

#endif
