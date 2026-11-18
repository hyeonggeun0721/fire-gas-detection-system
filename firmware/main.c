/*
 * 화재·가스 감지 자동 대피 유도 시스템 - 메인 진입점
 * 14단계: 통합 테스트 및 시나리오별 시뮬레이션 검증
 *
 * 핀 배치 (docs/architecture.md 참조)
 *   PA0: LED - 정상(녹색)   PA1: LED - 주의(노랑)   PA2: LED - 경보/대피(빨강)
 *   PB5(OC1A): 서보모터 (비상구)
 *   PC0-PC5: LCD (RS, E, D4-D7)
 *   PD0(INT0): 불꽃 센서 (풀업, 활성 LOW, 하강 엣지 인터럽트)
 *   PD1: 테스트 버튼 (풀업, 활성 LOW) - 강제로 정상 상태로 리셋
 *   PD4: 부저
 *   PE1(TXD0) / PE0(RXD0): UART 송수신 (PC 관제실)
 *   PF0(ADC0): 온도 센서   PF1(ADC1): 가스 센서
 *
 * 매 5ms 틱(Timer0)마다 온도/가스 센서를 읽고, 불꽃 감지 플래그와 함께
 * fire_evaluate()(fire_logic.c, docs/requirements.md 5절 기준)에 전달해 다음
 * 상태(정상/주의/경보/대피)를 계산한다. 상태가 바뀔 때만 LED/LCD/서보를
 * 갱신한다. 경보/대피 상태의 LED는 하드웨어 예산(2단계 핀맵)상 동일한
 * LED_ALARM 하나를 공유하며, LCD 문구와 부저 패턴, 서보 개폐로 구분한다.
 * 테스트 버튼은 시뮬레이션 시나리오를 위해 강제로 정상 상태로 리셋한다.
 * 경보/대피 상태에 진입할 때 UART로 PC 관제실에 알림 메시지를 전송한다.
 * 관제실 PC는 한 글자 명령으로 수신반을 원격 조작할 수 있다 (handle_command):
 *   S 상태 조회 / R 원격 복구 / T 화재 시험 / O 비상구 열기 / C 비상구 닫기
 *
 * 부저는 buzzer_tick()으로 매 5ms 틱마다 논블로킹으로 재생한다(14단계에서
 * _delay_ms() 기반 blocking beep()을 교체 - docs/test-scenarios.md 참조).
 *
 * 참고: HD44780 내장 폰트 ROM(A00)은 한글을 지원하지 않으므로 LCD 표시 문구는
 * 영문으로 작성한다. UART 관제실 메시지도 SimulIDE 시리얼 터미널이 ASCII로
 * 표시하므로 영문으로 작성한다.
 *
 * 시뮬레이션 트러블슈팅: SimulIDE 1.1.0의 ATmega128 모델은 호출 시 복귀 주소를
 * 3바이트로 스택에 넣는다(실제 칩은 2바이트). avr-gcc -Os가 작은 지역 변수 공간을
 * "rcall .+0"(복귀 주소를 밀어 넣는 방식)으로 확보한 함수는 끝에서 pop 개수가
 * 맞지 않아 엉뚱한 주소로 복귀하고, MCU가 처음부터 다시 시작하는 일이 반복됐다.
 * 그래서 snprintf()를 append_uint()/append_int()로 바꾸고, 지역 배열과
 * 주소를 넘기는 지역 변수를 없앴다(static). 빌드 결과에 "rcall .+0"이 0개인지
 * 확인한다 (docs/troubleshooting-simulide.md 참조).
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "debounce.h"
#include "lcd.h"
#include "adc.h"
#include "temp.h"
#include "gas.h"
#include "servo.h"
#include "fire_logic.h"
#include "uart.h"
#include "buzzer.h"

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
    buzzer_init();

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

/* 부호 없는 정수를 10진수 문자열로 p 위치에 붙이고, 다음 쓸 위치를 돌려준다.
 * 지역 배열을 쓰지 않는다: avr-gcc -Os는 작은 지역 변수 공간을 "rcall .+0"로
 * 확보하는데, SimulIDE의 ATmega128 모델은 이 rcall이 3바이트를 밀어 넣어
 * 함수 끝의 pop 개수와 맞지 않게 된다 (docs/troubleshooting-simulide.md). */
static char *append_uint(char *p, uint16_t v)
{
    uint16_t div = 10000;
    uint8_t started = 0;
    while (div > 1) {
        uint8_t d = (uint8_t)(v / div);
        if (d || started) {
            *p++ = (char)('0' + d);
            started = 1;
        }
        v %= div;
        div /= 10;
    }
    *p++ = (char)('0' + v);
    return p;
}

static char *append_int(char *p, int16_t v)
{
    if (v < 0) {
        *p++ = '-';
        return append_uint(p, (uint16_t)(-v));
    }
    return append_uint(p, (uint16_t)v);
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
            buzzer_stop();  /* 리셋으로 정상 복귀할 때 울리던 부저를 바로 끈다 */
            lcd_string("STATUS: NORMAL");
            lcd_set_cursor(1, 0);
            /* "T:25C G:10%" 형식 (최대 "T:-99C G:100%" = 13자) */
            char buf[17];
            char *p = buf;
            *p++ = 'T'; *p++ = ':';
            p = append_int(p, temp_c);
            *p++ = 'C'; *p++ = ' '; *p++ = 'G'; *p++ = ':';
            p = append_uint(p, gas_pct);
            *p++ = '%';
            *p = '\0';
            lcd_string(buf);
            break;
        }
        case FIRE_STATE_WARNING:
            LED_PORT |= LED_WARNING;
            servo_close();
            buzzer_start(1);
            lcd_string("STATUS: WARNING");
            lcd_set_cursor(1, 0);
            lcd_string("High temp!");
            break;
        case FIRE_STATE_ALARM:
            LED_PORT |= LED_ALARM;
            servo_close();
            buzzer_start(1);
            lcd_string("!! ALARM !!");
            lcd_set_cursor(1, 0);
            lcd_string("Stand by");
            uart_puts("[ALARM] Gas/temperature over threshold or flame detected\r\n");
            break;
        case FIRE_STATE_EVACUATE:
            LED_PORT |= LED_ALARM;
            servo_open();
            buzzer_start(3);
            lcd_string("!! EVACUATE !!");
            lcd_set_cursor(1, 0);
            lcd_string("EXIT NOW");
            uart_puts("[EVACUATE] Fire confirmed, exit door opened - evacuate now\r\n");
            break;
        default:
            break;
    }
}

static const char *state_name(fire_state_t state)
{
    switch (state)
    {
        case FIRE_STATE_NORMAL:   return "NORMAL";
        case FIRE_STATE_WARNING:  return "WARNING";
        case FIRE_STATE_ALARM:    return "ALARM";
        case FIRE_STATE_EVACUATE: return "EVACUATE";
        default:                  return "?";
    }
}

/* 관제실 PC가 보낸 한 글자 명령을 처리하고, 바뀐 상태를 돌려준다.
 * 회신 문자열은 static 버퍼에 만든다 (지역 배열 -> rcall .+0 회피). */
static fire_state_t handle_command(char cmd, fire_state_t state, int16_t temp_c,
                                   uint8_t gas_pct, uint16_t *alarm_ticks)
{
    static char reply[40];
    char *p;

    switch (cmd)
    {
        case 'S': case 's':  /* 상태 조회: "STATE=NORMAL T=19C G=9%" */
            p = reply;
            for (const char *n = "STATE="; *n; n++) *p++ = *n;
            for (const char *n = state_name(state); *n; n++) *p++ = *n;
            *p++ = ' '; *p++ = 'T'; *p++ = '=';
            p = append_int(p, temp_c);
            *p++ = 'C'; *p++ = ' '; *p++ = 'G'; *p++ = '=';
            p = append_uint(p, gas_pct);
            *p++ = '%'; *p++ = '\r'; *p++ = '\n';
            *p = '\0';
            uart_puts(reply);
            break;
        case 'R': case 'r':  /* 원격 복구: 복구 RESET 버튼과 같은 동작 */
            uart_puts("OK RESET\r\n");
            state = FIRE_STATE_NORMAL;
            *alarm_ticks = 0;
            show_state(state, temp_c, gas_pct);
            break;
        case 'T': case 't':  /* 화재 시험: 경보 상태로 전환 (3초 뒤 대피, R로 해제) */
            if (state == FIRE_STATE_ALARM || state == FIRE_STATE_EVACUATE)
            {
                uart_puts("ERR ALREADY ALARM\r\n");
                break;
            }
            uart_puts("OK TEST -> ALARM\r\n");
            state = FIRE_STATE_ALARM;
            *alarm_ticks = 0;
            show_state(state, temp_c, gas_pct);
            break;
        case 'O': case 'o':  /* 비상구 수동 열기 */
            servo_open();
            uart_puts("OK DOOR OPEN\r\n");
            break;
        case 'C': case 'c':  /* 비상구 수동 닫기: 대피 중에는 거부 (대피로 확보가 우선) */
            if (state == FIRE_STATE_EVACUATE)
            {
                uart_puts("ERR EVACUATING - DOOR STAYS OPEN\r\n");
                break;
            }
            servo_close();
            uart_puts("OK DOOR CLOSE\r\n");
            break;
        case '\r': case '\n': case ' ':  /* Enter/공백은 무시 */
            break;
        default:
            uart_puts("ERR ? (S/R/T/O/C)\r\n");
            break;
    }
    return state;
}

int main(void)
{
    gpio_init();

    fire_state_t state = FIRE_STATE_NORMAL;
    static uint16_t alarm_ticks = 0;  /* static: &alarm_ticks로 넘기므로 스택 대신 정적 메모리 */
    static debounce_t button_db;  /* static: 스택 프레임(rcall .+0) 대신 정적 메모리 사용 */
    debounce_init(&button_db, 1);  /* 풀업으로 유휴 상태는 HIGH */
    uint8_t last_stable = 1;

    /* 초기 화면에 실제 센서값을 표시하도록, 첫 틱을 기다리지 않고 한 번 읽어둔다 */
    int16_t initial_temp_c = adc_to_temp_c(adc_read(TEMP_ADC_CHANNEL));
    uint8_t initial_gas_pct = adc_to_gas_percent(adc_read(GAS_ADC_CHANNEL));
    show_state(state, initial_temp_c, initial_gas_pct);

    sei();  /* 전역 인터럽트 활성화 */

    while (1)
    {
        while (!tick_flag)
        {
            /* Timer0 비교 일치 인터럽트가 다음 틱을 알릴 때까지 대기 */
        }
        tick_flag = 0;

        buzzer_tick();  /* 논블로킹으로 부저 패턴을 한 틱 진행 */

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

        /* 관제실 PC 명령 처리 (한 틱에 받은 글자를 모두 처리) */
        static char rx;  /* static: 주소를 넘기는 변수 -> 스택 프레임(rcall .+0) 회피 */
        while (uart_getc_nb(&rx))
        {
            state = handle_command(rx, state, temp_c, gas_pct, &alarm_ticks);
        }
    }
}
