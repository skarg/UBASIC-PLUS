#include <stdint.h>
#include <stdio.h>
#include "ubasic/ubasic.h"

void analogWriteConfig(uint16_t psc, uint16_t per)
{
    printf("analogWriteConfig(%d, %d)\n", psc, per);
}

void analogWrite(uint8_t ch, int16_t dutycycle)
{
    printf("analogWrite(%d, %d)\n", ch, dutycycle);
}

void analogReadConfig(uint8_t sampletime, uint8_t nreads)
{
    printf("analogReadConfig(%d, %d)\n", sampletime, nreads);
}

int16_t analogRead(uint8_t channel)
{
    printf("analogRead(%d)\n", channel);
    return 0;
}
