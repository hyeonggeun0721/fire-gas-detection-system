/*
 * 서보모터 PWM 제어 (비상구 개폐)
 * Timer1, OC1A(PB5), Fast PWM(TOP=ICR1), 분주비 8, 16MHz 기준 50Hz 주기
 */
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

#define SERVO_TOP            39999U /* ICR1 TOP: 16MHz/8/50Hz - 1 -> 20ms 주기 */
#define SERVO_TICKS_PER_US   2U     /* 16MHz/8 = 2MHz -> 마이크로초당 2틱 */
#define SERVO_PULSE_MIN_US   1000U  /* 0도(완전 닫힘) 펄스폭 */
#define SERVO_PULSE_MAX_US   2000U  /* 180도(완전 열림) 펄스폭 */
#define SERVO_ANGLE_MAX_DEG  180U

/* angle_deg(0~180)를 OCR1A에 넣을 타이머 비교값(카운트)으로 변환한다.
 * AVR 레지스터에 의존하지 않는 순수 함수라 호스트에서도 단위 테스트 가능 */
uint16_t servo_angle_to_ocr1a(uint8_t angle_deg);

void servo_init(void);
void servo_open(void);   /* 비상구 개방 */
void servo_close(void);  /* 비상구 닫힘 */

#endif
