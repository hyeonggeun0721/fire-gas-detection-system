#include <avr/io.h>

#include "servo.h"

#define SERVO_CLOSED_ANGLE_DEG 0
#define SERVO_OPEN_ANGLE_DEG   90

uint16_t servo_angle_to_ocr1a(uint8_t angle_deg)
{
    if (angle_deg > SERVO_ANGLE_MAX_DEG) angle_deg = SERVO_ANGLE_MAX_DEG;

    /* 각도를 먼저 서보의 유효 펄스 범위(1000~2000us)로 선형 변환한 뒤,
     * 그 펄스 폭을 타이머 카운트(마이크로초당 SERVO_TICKS_PER_US틱)로 환산한다.
     * 서보는 20ms 주기 전체가 아니라 이 좁은 펄스 폭만으로 각도를 해석한다. */
    uint32_t pulse_us = SERVO_PULSE_MIN_US +
        ((uint32_t)angle_deg * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / SERVO_ANGLE_MAX_DEG;

    return (uint16_t)(pulse_us * SERVO_TICKS_PER_US);
}

void servo_init(void)
{
    DDRB |= (1 << PB5);  /* OC1A 출력 */

    ICR1 = SERVO_TOP;
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  /* Fast PWM, TOP=ICR1, 분주비 8 */

    servo_close();
}

void servo_open(void)
{
    OCR1A = servo_angle_to_ocr1a(SERVO_OPEN_ANGLE_DEG);
}

void servo_close(void)
{
    OCR1A = servo_angle_to_ocr1a(SERVO_CLOSED_ANGLE_DEG);
}
