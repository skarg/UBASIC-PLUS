#include <stdint.h>
#include <stdio.h>
#include "ubasic/ubasic.h"

void pinMode(uint8_t ch, int8_t mode, uint8_t freq)
{
    printf("pinMode(%d, %d, %d)\n", ch, mode, freq);
}

int8_t digitalWrite(uint8_t ch, uint8_t PinState)
{
    printf("digitalWrite(%d, %d)\n", ch, PinState);
    return 0;
}

int8_t digitalRead(uint8_t ch)
{
    printf("digitalRead(%d)\n", ch);
    return 0;
}
