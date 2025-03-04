#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ubasic/ubasic.h"

static bool Initialized = false;

/**
 * @brief Generate a random number
 * @param size Size of the random number in bits
 */
uint32_t RandomUInt32(uint8_t size)
{
    uint32_t value = 0, grains = 0;
    uint8_t k, i;

    if (!Initialized)
    {
        srand(0);
        Initialized = true;
    }
    for (k = 0; k < 4; k++)
    {
        grains = 0;
        for (i = 0; i < (size >> 1); i++)
        {
            /* Two LS bits are most likely most random */
            grains |= (rand() & 0x00000003) << (2 * i);
        }
        value ^= grains;
    }

    return value;
}
