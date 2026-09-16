#include <avr/io.h>

#include "servo.h"

#define SERVO_CLOSED_ANGLE_DEG 0
#define SERVO_OPEN_ANGLE_DEG   90

uint16_t servo_angle_to_ocr1a(uint8_t angle_deg)
{
    if (angle_deg > SERVO_ANGLE_MAX_DEG) angle_deg = SERVO_ANGLE_MAX_DEG;

    /* BUG: 각도를 PWM 전체 주기(SERVO_TOP, 20ms)에 대한 듀티비로 직접
     * 매핑했다. 서보모터는 20ms 주기 전체가 아니라 그 안의 1~2ms 펄스
     * 폭만으로 각도를 해석하므로, 이 계산은 유효 범위를 크게 벗어난
     * 펄스 폭(예: 90도 -> 약 10ms)을 만들어낸다. */
    return (uint16_t)(((uint32_t)angle_deg * SERVO_TOP) / SERVO_ANGLE_MAX_DEG);
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
