/*
 * 화재·가스 감지 자동 대피 유도 시스템 - 메인 진입점
 * 13단계: UART 통신 - PC 관제실로 신고 메시지 전송
 *
 * 핀 배치 (docs/architecture.md 참조)
 *   PA0: LED - 정상(녹색)   PA1: LED - 주의(노랑)   PA2: LED - 경보/대피(빨강)
 *   PB5(OC1A): 서보모터 (비상구)
 *   PC0-PC5: LCD (RS, E, D4-D7)
 *   PD0(INT0): 불꽃 센서 (풀업, 활성 LOW, 하강 엣지 인터럽트)
 *   PD1: 테스트 버튼 (풀업, 활성 LOW) - 강제로 정상 상태로 리셋
 *   PD4: 부저
 *   PE1(TXD0): UART 송신 (PC 관제실)
 *   PF0(ADC0): 온도 센서   PF1(ADC1): 가스 센서
 *
 * 매 5ms 틱(Timer0)마다 온도/가스 센서를 읽고, 불꽃 감지 플래그와 함께
 * fire_evaluate()(fire_logic.c, docs/requirements.md 5절 기준)에 전달해 다음
 * 상태(정상/주의/경보/대피)를 계산한다. 상태가 바뀔 때만 LED/LCD/부저/서보를
 * 갱신한다. 경보/대피 상태의 LED는 하드웨어 예산(2단계 핀맵)상 동일한
 * LED_ALARM 하나를 공유하며, LCD 문구와 부저 패턴, 서보 개폐로 구분한다.
 * 테스트 버튼은 시뮬레이션 시나리오를 위해 강제로 정상 상태로 리셋한다.
 * 경보/대피 상태에 진입할 때 UART로 PC 관제실에 알림 메시지를 전송한다.
 *
 * 참고: HD44780 내장 폰트 ROM(A00)은 한글을 지원하지 않으므로 LCD 표시 문구는
 * 영문으로 작성하지만, UART는 PC 터미널이 UTF-8을 그대로 표시할 수 있으므로
 * 관제실 메시지는 한글로 작성한다.
 */

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "debounce.h"
#include "lcd.h"
#include "adc.h"
#include "temp.h"
#include "gas.h"
#include "servo.h"
#include "fire_logic.h"
#include "uart.h"

#define TEMP_ADC_CHANNEL 0
#define GAS_ADC_CHANNEL  1

#define FLAME_DDR  DDRD
#define FLAME_PORT PORTD
#define FLAME_PIN  (1 << PD0)

/* Timer0 CTC 기반 5ms(200Hz) 틱 생성.
 * 프리스케일러 1024를 사용해야 16MHz에서 5ms 주기의 비교값(OCR0)이
 * 8비트 레지스터 범위(0~255) 안에 들어온다. */
#define TIMER0_PRESCALER 1024
#define TIMER0_TARGET_HZ 200  /* 5ms 주기 */
#define TIMER0_OCR (F_CPU / TIMER0_PRESCALER / TIMER0_TARGET_HZ - 1)

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

/* ISR에서 설정되고 메인 루프에서 읽는 값이므로 volatile로 선언 */
static volatile uint8_t flame_detected = 0;
static volatile uint8_t tick_flag = 0;

ISR(INT0_vect)
{
    flame_detected = 1;
}

ISR(TIMER0_COMP_vect)
{
    tick_flag = 1;
}

static void timer0_init(void)
{
    OCR0 = TIMER0_OCR;
    TCCR0 = (1 << WGM01) | (1 << CS02) | (1 << CS01) | (1 << CS00);  /* CTC 모드, 분주비 1024 (CS02:00 = 111) */
    TIMSK |= (1 << OCIE0);               /* 비교 일치 인터럽트 활성화 */
}

static void gpio_init(void)
{
    BUZZER_DDR |= BUZZER_PIN;
    BUZZER_PORT &= ~BUZZER_PIN;

    LED_DDR |= LED_ALL;
    LED_PORT &= ~LED_ALL;

    BUTTON_DDR &= ~BUTTON_PIN;  /* 입력 */
    BUTTON_PORT |= BUTTON_PIN;  /* 내부 풀업 활성화 */

    FLAME_DDR &= ~FLAME_PIN;    /* 입력 */
    FLAME_PORT |= FLAME_PIN;    /* 내부 풀업 활성화 (센서는 불꽃 감지 시 LOW 출력) */
    EICRA |= (1 << ISC01);      /* ISC01:00 = 10 -> 하강 엣지에서 인터럽트 발생 */
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);       /* INT0 인터럽트 활성화 */

    lcd_init();
    adc_init();
    timer0_init();
    servo_init();
    uart_init();
}

static void beep(uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        BUZZER_PORT |= BUZZER_PIN;
        _delay_ms(100);
        BUZZER_PORT &= ~BUZZER_PIN;
        _delay_ms(100);
    }
}

static void show_state(fire_state_t state, int16_t temp_c, uint8_t gas_pct)
{
    LED_PORT &= ~LED_ALL;
    lcd_clear();
    lcd_set_cursor(0, 0);

    switch (state)
    {
        case FIRE_STATE_NORMAL:
        {
            LED_PORT |= LED_NORMAL;
            servo_close();
            lcd_string("STATUS: NORMAL");
            lcd_set_cursor(1, 0);
            char buf[17];
            snprintf(buf, sizeof(buf), "T:%dC G:%u%%", temp_c, gas_pct);
            lcd_string(buf);
            break;
        }
        case FIRE_STATE_WARNING:
            LED_PORT |= LED_WARNING;
            servo_close();
            beep(1);
            lcd_string("STATUS: WARNING");
            lcd_set_cursor(1, 0);
            lcd_string("High temp!");
            break;
        case FIRE_STATE_ALARM:
            LED_PORT |= LED_ALARM;
            servo_close();
            beep(1);
            lcd_string("!! ALARM !!");
            lcd_set_cursor(1, 0);
            lcd_string("Stand by");
            uart_puts("[경보] 가스 또는 온도 임계값 초과, 혹은 불꽃 감지\r\n");
            break;
        case FIRE_STATE_EVACUATE:
            LED_PORT |= LED_ALARM;
            servo_open();
            beep(3);
            lcd_string("!! EVACUATE !!");
            lcd_set_cursor(1, 0);
            lcd_string("EXIT NOW");
            uart_puts("[대피] 화재 확정, 비상구 개방 - 즉시 대피 바랍니다\r\n");
            break;
        default:
            break;
    }
}

int main(void)
{
    gpio_init();

    fire_state_t state = FIRE_STATE_NORMAL;
    uint16_t alarm_ticks = 0;
    debounce_t button_db;
    debounce_init(&button_db, 1);  /* 풀업으로 유휴 상태는 HIGH */
    uint8_t last_stable = 1;
    show_state(state, 0, 0);

    sei();  /* 전역 인터럽트 활성화 */

    while (1)
    {
        while (!tick_flag)
        {
            /* Timer0 비교 일치 인터럽트가 다음 틱을 알릴 때까지 대기 */
        }
        tick_flag = 0;

        int16_t temp_c = adc_to_temp_c(adc_read(TEMP_ADC_CHANNEL));
        uint8_t gas_pct = adc_to_gas_percent(adc_read(GAS_ADC_CHANNEL));

        uint8_t flame = flame_detected;
        flame_detected = 0;  /* 이번 틱에서 소비했으므로 리셋 */

        fire_state_t next = fire_evaluate(state, temp_c, gas_pct, flame, &alarm_ticks);
        if (next != state)
        {
            state = next;
            show_state(state, temp_c, gas_pct);
        }

        uint8_t raw = (BUTTON_PIN_REG & BUTTON_PIN) ? 1 : 0;
        uint8_t stable = debounce_update(&button_db, raw);

        /* 디바운싱된 안정 신호에서 눌림(HIGH->LOW) 엣지만 반응: 강제로 정상 상태 리셋 */
        if (last_stable == 1 && stable == 0)
        {
            state = FIRE_STATE_NORMAL;
            alarm_ticks = 0;
            show_state(state, temp_c, gas_pct);
        }
        last_stable = stable;
    }
}
