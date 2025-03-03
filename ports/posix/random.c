#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "ubasic/ubasic.h"

static bool Initialized = false;

/**
 * @brief Generate a random number
 */
uint32_t RandomUInt32(uint8_t size)
{
    (void)size;
    if (!Initialized)
    {
        srand(0);
        Initialized = true;
    }

    return (uint32_t)rand();
}
