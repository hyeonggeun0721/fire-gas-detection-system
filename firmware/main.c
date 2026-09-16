/*
 * 화재·가스 감지 자동 대피 유도 시스템 - 메인 진입점
 * 7단계: ADC 온도 센서 읽기
 *
 * 핀 배치 (docs/architecture.md 참조)
 *   PA0: LED - 정상(녹색)   PA1: LED - 주의(노랑)   PA2: LED - 경보(빨강)
 *   PC0-PC5: LCD (RS, E, D4-D7)
 *   PD1: 테스트 버튼 (풀업, 활성 LOW)
 *   PD4: 부저
 *   PF0(ADC0): 온도 센서
 *
 * 테스트 버튼을 누를 때마다 정상 -> 주의 -> 경보 -> 정상 순으로 상태가 순환하며
 * LCD에도 현재 상태 메시지를 표시한다. 정상 상태로 진입할 때마다 온도 센서를
 * 읽어 LCD 2행에 표시한다. (실제 센서 기반 자동 상태 전이는 12단계에서 구현)
 *
 * 참고: HD44780 내장 폰트 ROM(A00)은 한글을 지원하지 않으므로 LCD 표시 문구는
 * 영문으로 작성한다.
 */

#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>

#include "debounce.h"
#include "lcd.h"
#include "adc.h"
#include "temp.h"

#define TEMP_ADC_CHANNEL 0

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

    lcd_init();
    adc_init();
}

static void show_state(system_state_t state)
{
    LED_PORT &= ~LED_ALL;
    lcd_clear();
    lcd_set_cursor(0, 0);

    switch (state)
    {
        case STATE_NORMAL:
        {
            LED_PORT |= LED_NORMAL;
            lcd_string("STATUS: NORMAL");
            lcd_set_cursor(1, 0);

            int16_t temp_c = adc_to_temp_c(adc_read(TEMP_ADC_CHANNEL));
            char buf[17];
            snprintf(buf, sizeof(buf), "TEMP: %d C", temp_c);
            lcd_string(buf);
            break;
        }
        case STATE_WARNING:
            LED_PORT |= LED_WARNING;
            lcd_string("STATUS: WARNING");
            lcd_set_cursor(1, 0);
            lcd_string("Check sensors");
            break;
        case STATE_ALARM:
            LED_PORT |= LED_ALARM;
            BUZZER_PORT |= BUZZER_PIN;
            _delay_ms(100);
            BUZZER_PORT &= ~BUZZER_PIN;
            lcd_string("!! FIRE ALARM !!");
            lcd_set_cursor(1, 0);
            lcd_string("EVACUATE NOW");
            break;
        default:
            break;
    }
}

int main(void)
{
    gpio_init();

    system_state_t state = STATE_NORMAL;
    debounce_t button_db;
    debounce_init(&button_db, 1);  /* 풀업으로 유휴 상태는 HIGH */
    uint8_t last_stable = 1;
    show_state(state);

    while (1)
    {
        uint8_t raw = (BUTTON_PIN_REG & BUTTON_PIN) ? 1 : 0;
        uint8_t stable = debounce_update(&button_db, raw);

        /* 디바운싱된 안정 신호에서 눌림(HIGH->LOW) 엣지만 반응 */
        if (last_stable == 1 && stable == 0)
        {
            state = (system_state_t)((state + 1) % STATE_COUNT);
            show_state(state);
        }
        last_stable = stable;

        _delay_ms(5);
    }
}
