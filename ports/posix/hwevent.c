#include <stdint.h>
#include "ubasic/ubasic.h"

static uint32_t Event_Mask;

int8_t hw_event(uint8_t bit)
{
    if (bit < 32) {
        if (Event_Mask & (1UL << bit)) {
            return 1; // Event is set
        }
    }

    return 0; // Event is not set
}

void hw_event_clear(uint8_t bit)
{
    if (bit < 32) {
        Event_Mask &= ~(1UL << bit);
    }
}

void hw_event_set(uint8_t bit)
{
    if (bit < 32) {
        Event_Mask |= (1UL << bit);
    }
}
