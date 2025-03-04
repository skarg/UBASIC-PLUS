#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "ubasic/ubasic.h"

/* start time for the clock */
static struct timespec Start;
static bool Initialized;
#ifndef CONFIG_UBASIC_TIMER_TIC_TOC_MAX
#define CONFIG_UBASIC_TIMER_TIC_TOC_MAX 128
#endif

/**
 * @brief Retrieves the system time, in milliseconds.
 * @return The system time, in milliseconds.
 */
uint32_t timer_now(void)
{
    struct timespec now;
    uint32_t ticks;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (Initialized)
    {
        ticks = (now.tv_sec - Start.tv_sec) * 1000L +
                (now.tv_nsec - Start.tv_nsec) / 1000000L;
    }
    else
    {
        Start.tv_sec = now.tv_sec;
        Start.tv_nsec = now.tv_nsec;
        ticks = 0;
        Initialized = true;
    }

    return ticks;
}

/**
 * @brief Calculates the elapsed time since the given start time.
 * @param start The start time.
 * @return The elapsed time, in milliseconds.
 */
uint32_t timer_since(uint32_t start)
{
    return timer_now() - start;
}
