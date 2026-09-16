/*
 * 화재·가스 감지 자동 대피 유도 시스템 - 메인 진입점
 * 5단계: 테스트 버튼 입력 및 디바운싱
 *
 * 핀 배치 (docs/architecture.md 참조)
 *   PA0: LED - 정상(녹색)   PA1: LED - 주의(노랑)   PA2: LED - 경보(빨강)
 *   PD1: 테스트 버튼 (풀업, 활성 LOW)
 *   PD4: 부저
 *
 * 테스트 버튼을 누를 때마다 정상 -> 주의 -> 경보 -> 정상 순으로 상태가 순환한다.
 * (실제 센서 기반 상태 전이는 12단계에서 구현)
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

#define BUTTON_DDR      DDRD
#define BUTTON_PORT     PORTD
#define BUTTON_PIN_REG  PIND
#define BUTTON_PIN      (1 << PD1)

typedef enum { STATE_NORMAL = 0, STATE_WARNING, STATE_ALARM, STATE_COUNT } system_state_t;

static void gpio_init(void)
{
    BUZZER_DDR |= BUZZER_PIN;
    BUZZER_PORT &= ~BUZZER_PIN;

    LED_DDR |= LED_ALL;
    LED_PORT &= ~LED_ALL;

    BUTTON_DDR &= ~BUTTON_PIN;  /* 입력 */
    BUTTON_PORT |= BUTTON_PIN;  /* 내부 풀업 활성화 */
}

static void show_state(system_state_t state)
{
    LED_PORT &= ~LED_ALL;
    switch (state)
    {
        case STATE_NORMAL:
            LED_PORT |= LED_NORMAL;
            break;
        case STATE_WARNING:
            LED_PORT |= LED_WARNING;
            break;
        case STATE_ALARM:
            LED_PORT |= LED_ALARM;
            BUZZER_PORT |= BUZZER_PIN;
            _delay_ms(100);
            BUZZER_PORT &= ~BUZZER_PIN;
            break;
        default:
            break;
    }
}

int main(void)
{
    gpio_init();

    system_state_t state = STATE_NORMAL;
    uint8_t last_raw = 1;  /* 풀업으로 유휴 상태는 HIGH */
    show_state(state);

    while (1)
    {
        uint8_t raw = (BUTTON_PIN_REG & BUTTON_PIN) ? 1 : 0;

        /* 버튼 눌림(HIGH->LOW) 감지 - 디바운싱 없이 원시 신호로 바로 판단 */
        if (last_raw == 1 && raw == 0)
        {
            state = (system_state_t)((state + 1) % STATE_COUNT);
            show_state(state);
        }
        last_raw = raw;

        _delay_ms(5);
    }
}
