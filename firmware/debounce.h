/*
 * 카운터 기반 디바운싱 모듈
 * AVR 레지스터에 의존하지 않는 순수 C 로직이라 호스트 환경에서도 단위 테스트 가능
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>

/* 원시 입력이 이 횟수만큼 연속으로 같은 값을 유지해야 상태 변경을 인정한다.
 * 샘플링 주기(main.c의 _delay_ms(5))와 곱하면 디바운싱 시간이 된다.
 * 4 * 5ms = 20ms (요구사항 명세서 6절의 최소 디바운싱 시간과 일치) */
#define DEBOUNCE_THRESHOLD 4

typedef struct
{
    uint8_t stable_state;
    uint8_t counter;
} debounce_t;

void debounce_init(debounce_t *db, uint8_t initial_state);

/* raw_state: 이번 샘플링 주기에 읽은 원시 입력값(0 또는 1)
 * 반환값: 디바운싱을 거친 안정된 상태값 */
uint8_t debounce_update(debounce_t *db, uint8_t raw_state);

#endif
