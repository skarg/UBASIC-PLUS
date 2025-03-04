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
static uint32_t Basic_Timer[CONFIG_UBASIC_TIMER_TIC_TOC_MAX];

struct timer_wait {
    uint32_t start;
    uint32_t duration;
};
static struct timer_wait Input_Wait_Timer;
static struct timer_wait Sleep_Timer;

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
static uint32_t timer_since(uint32_t start)
{
    return timer_now() - start;
}

void timer_tic(uint8_t ch)
{
    if (ch > CONFIG_UBASIC_TIMER_TIC_TOC_MAX) {
        return;
    }
    Basic_Timer[ch] = timer_now();
}

int32_t timer_toc(uint8_t ch)
{
    uint32_t elapsed;
    if (ch > CONFIG_UBASIC_TIMER_TIC_TOC_MAX) {
        return 0;
    }
    elapsed = timer_since(Basic_Timer[ch]);
    if (elapsed > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32_t)elapsed;
}

void timer_input_wait(int32_t ms)
{
    Input_Wait_Timer.duration = ms;
    Input_Wait_Timer.start = timer_now();
}

int32_t timer_input_remaining(void)
{
    uint32_t remaining;
    uint32_t elapsed;

    if (Input_Wait_Timer.duration > 0) {
        elapsed = timer_since(Input_Wait_Timer.start);
        if (elapsed < Input_Wait_Timer.duration) {
            remaining = Input_Wait_Timer.duration - elapsed;
            if (remaining > INT32_MAX) {
                return INT32_MAX;
            }
            return (int32_t)remaining;
        } else {
            Input_Wait_Timer.duration = 0;
        }
    }

    return 0;
}

void timer_sleep(int32_t ms)
{
    Sleep_Timer.duration = ms;
    Sleep_Timer.start = timer_now();
}

int32_t timer_sleeping(void)
{
    uint32_t remaining;
    uint32_t elapsed;

    if (Sleep_Timer.duration > 0) {
        elapsed = timer_since(Sleep_Timer.start);
        if (elapsed < Sleep_Timer.duration) {
            remaining = Sleep_Timer.duration - elapsed;
            if (remaining > INT32_MAX) {
                return INT32_MAX;
            }
            return (int32_t)remaining;
        } else {
            Sleep_Timer.duration = 0;
        }
    }

    return 0;
}
