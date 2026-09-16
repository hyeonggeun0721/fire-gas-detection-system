#include "debounce.h"

void debounce_init(debounce_t *db, uint8_t initial_state)
{
    db->stable_state = initial_state;
    db->counter = 0;
}

uint8_t debounce_update(debounce_t *db, uint8_t raw_state)
{
    if (raw_state == db->stable_state)
    {
        db->counter = 0;
        return db->stable_state;
    }

    db->counter++;
    if (db->counter >= DEBOUNCE_THRESHOLD)
    {
        db->stable_state = raw_state;
        db->counter = 0;
    }
    return db->stable_state;
}
