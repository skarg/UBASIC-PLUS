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
#define CONFIG_UBASIC_TIMER_TIC_TOC_MAX 8
#endif
static uint32_t Basic_Timer[CONFIG_UBASIC_TIMER_TIC_TOC_MAX];

static struct timer_wait {
    uint32_t start;
    uint32_t duration;
} Input_Wait_Timer;


/**
 * @brief Retrieves the system time, in milliseconds.
 * @return The system time, in milliseconds.
 */
static uint32_t timer_now(void)
{
    struct timespec now;
    uint32_t ticks;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (Initialized)
    {
        ticks = (now.tv_sec - Start.tv_sec) * 1000UL +
                (now.tv_nsec - Start.tv_nsec) / 1000000UL;
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
static uint32_t timer_since(uint32_t start)
{
    return timer_now() - start;
    return 0;
}

void timer_tic(uint8_t ch)
{
    if (ch > CONFIG_UBASIC_TIMER_TIC_TOC_MAX) {
        return;
    }
    Basic_Timer[ch] = timer_now();
}

uint32_t timer_toc(uint8_t ch)
{
    if (ch > CONFIG_UBASIC_TIMER_TIC_TOC_MAX) {
        return 0;
    }
    return timer_since(Basic_Timer[ch]);
}

void timer_input_wait(uint32_t ms)
{
    Input_Wait_Timer.duration = ms;
    Input_Wait_Timer.start = timer_now();
}

uint32_t timer_input_remaining(void)
{
    uint32_t elapsed = timer_since(Input_Wait_Timer.start);
    if (elapsed < Input_Wait_Timer.duration) {
        return Input_Wait_Timer.duration - elapsed;
    }
    return 0;
}
