#include <avr/io.h>

#include "buzzer.h"

#define BUZZER_PORT PORTD
#define BUZZER_DDR  DDRD
#define BUZZER_PIN  (1 << PD4)

static buzzer_seq_t s_seq;

void buzzer_init(void)
{
    BUZZER_DDR |= BUZZER_PIN;
    BUZZER_PORT &= ~BUZZER_PIN;
    s_seq.phase = BUZZER_PHASE_IDLE;
}

void buzzer_start(uint8_t beep_count)
{
    buzzer_seq_start(&s_seq, beep_count);
}

void buzzer_stop(void)
{
    buzzer_seq_start(&s_seq, 0);
    BUZZER_PORT &= ~BUZZER_PIN;
}

void buzzer_tick(void)
{
    if (buzzer_seq_tick(&s_seq))
    {
        BUZZER_PORT |= BUZZER_PIN;
    }
    else
    {
        BUZZER_PORT &= ~BUZZER_PIN;
    }
}
