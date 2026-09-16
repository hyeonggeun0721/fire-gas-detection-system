/*
 * 화재·가스 감지 자동 대피 유도 시스템 - 메인 진입점
 * 4단계: GPIO 기초 - 상태 LED / 부저 출력 제어
 *
 * 핀 배치 (docs/architecture.md 참조)
 *   PA0: LED - 정상(녹색)   PA1: LED - 주의(노랑)   PA2: LED - 경보(빨강)
 *   PD4: 부저
 */

#include <avr/io.h>
#include <util/delay.h>

#define LED_PORT PORTA
#define LED_DDR  DDRA
#define LED_NORMAL   (1 << PA0)
#define LED_WARNING  (1 << PA1)
#define LED_ALARM    (1 << PA2)
#define LED_ALL      (LED_NORMAL | LED_WARNING | LED_ALARM)

#define BUZZER_PORT PORTD
#define BUZZER_DDR  DDRD
#define BUZZER_PIN  (1 << PD4)

static void gpio_init(void)
{
    /* 부저 핀은 출력으로 설정 */
    BUZZER_DDR |= BUZZER_PIN;
    BUZZER_PORT &= ~BUZZER_PIN;

    /* LED 포트는 항상 꺼진 상태로 시작 */
    LED_PORT &= ~LED_ALL;
}

static void leds_off(void)
{
    LED_PORT &= ~LED_ALL;
}

int main(void)
{
    gpio_init();

    while (1)
    {
        leds_off();
        LED_PORT |= LED_NORMAL;
        _delay_ms(1000);

        leds_off();
        LED_PORT |= LED_WARNING;
        _delay_ms(1000);

        leds_off();
        LED_PORT |= LED_ALARM;
        BUZZER_PORT |= BUZZER_PIN;
        _delay_ms(1000);
        BUZZER_PORT &= ~BUZZER_PIN;
    }
}
