/*
 * 서보 각도 -> OCR1A 변환 호스트 단위 테스트
 * servo.c의 servo_angle_to_ocr1a()와 동일한 두 계산 방식(버그/수정)을 하드웨어
 * 없이 재현해, 계산된 펄스 폭이 서보의 유효 제어 범위(1000~2000us)를
 * 벗어나는지 검증한다.
 * (11단계 버그: 각도를 PWM 전체 주기(20ms)에 대한 듀티비로 직접 매핑하면,
 *  1~2ms의 좁은 유효 펄스 범위를 크게 벗어나는 값이 나온다.)
 */
#include <stdio.h>
#include <stdint.h>

#define SERVO_TOP 39999U
#define SERVO_TICKS_PER_US 2U
#define SERVO_PULSE_MIN_US 1000U
#define SERVO_PULSE_MAX_US 2000U
#define SERVO_ANGLE_MAX_DEG 180U

/* Week11 기능 커밋의 버그: 각도를 전체 주기(SERVO_TOP)에 대한 듀티비로 직접 매핑 */
static uint16_t angle_to_ocr1a_buggy(uint8_t angle_deg)
{
    return (uint16_t)(((uint32_t)angle_deg * SERVO_TOP) / SERVO_ANGLE_MAX_DEG);
}

/* 수정 후: 각도를 1000~2000us 펄스 폭으로 먼저 변환한 뒤 타이머 카운트로 환산 */
static uint16_t angle_to_ocr1a_fixed(uint8_t angle_deg)
{
    uint32_t pulse_us = SERVO_PULSE_MIN_US +
        ((uint32_t)angle_deg * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / SERVO_ANGLE_MAX_DEG;
    return (uint16_t)(pulse_us * SERVO_TICKS_PER_US);
}

static double ocr_to_pulse_us(uint16_t ocr)
{
    return ocr / (double)SERVO_TICKS_PER_US;
}

int main(void)
{
    const uint8_t angles[] = {0, 90, 180};  /* 닫힘, 중간, 완전 개방 */
    int all_fixed_in_range = 1;
    int any_buggy_out_of_range = 0;

    for (unsigned i = 0; i < sizeof(angles) / sizeof(angles[0]); i++)
    {
        uint8_t angle = angles[i];
        double pulse_buggy = ocr_to_pulse_us(angle_to_ocr1a_buggy(angle));
        double pulse_fixed = ocr_to_pulse_us(angle_to_ocr1a_fixed(angle));

        printf("angle=%3u deg | 버그: %7.1f us | 수정: %7.1f us (유효 범위 %u~%u us)\n",
               angle, pulse_buggy, pulse_fixed, SERVO_PULSE_MIN_US, SERVO_PULSE_MAX_US);

        if (pulse_fixed < SERVO_PULSE_MIN_US || pulse_fixed > SERVO_PULSE_MAX_US)
        {
            all_fixed_in_range = 0;
        }
        if (pulse_buggy < SERVO_PULSE_MIN_US || pulse_buggy > SERVO_PULSE_MAX_US)
        {
            any_buggy_out_of_range = 1;
        }
    }

    if (!all_fixed_in_range)
    {
        printf("FAIL: 수정 로직인데도 유효 범위를 벗어난 각도가 있음\n");
        return 1;
    }
    if (!any_buggy_out_of_range)
    {
        printf("FAIL: 버그 시나리오에서 유효 범위 이탈이 재현되지 않음\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
