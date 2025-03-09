/*
 * Copyright (c) 2025, Steve Karg <skarg@users.sourceforge.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#if defined(_MSC_VER)
#include <conio.h> /* for kbhit and getch */
#else
#include <termios.h> /* used in kbhit() */
#include <sys/ioctl.h> /* used in kbhit() */
#endif
#include "ubasic/ubasic.h"

#include <ctype.h>
#if defined(_MSC_VER)
#include <conio.h> /* for kbhit and getch */
#else
#include <termios.h> /* used in kbhit() */
#include <sys/ioctl.h> /* used in kbhit() */
#endif
#include "ubasic/ubasic.h"

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
#if defined(__GNUC__)
/**
 * @brief Check if a key has been pressed
 * @return 1 if a key has been pressed, 0 otherwise
 */
static int kbhit(void)
{
    static const int STDIN = 0;
    static bool initialized = false;
    int bytesWaiting;

    if (!initialized) {
        /* use termios to turn off line buffering */
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
#endif

/**
 * @brief Write a buffer to the serial port
 * @param msg Pointer to the buffer to write
 * @param n Number of bytes to write
 */
static void posix_serial_write(const char *msg, uint16_t n)
{
    printf("%.*s", n, msg);
    fflush(stdout);
}

/**
 * @brief Gather key presses until new-line is recieved or buffer is full
 * @return return the next byte from the input stream, or EOF(-1) if no byte is
 *  available
 */
static int posix_serial_getc(void)
{
    int ch = -1;

    if (kbhit()) {
        ch = fgetc(stdin);
    }

    return ch;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
static uint32_t Event_Mask;

/**
 * @brief Hardware event status bit
 * @param bit Event bit
 * @return 1 if the event is set, 0 otherwise
 */
static int8_t posix_hw_event(uint8_t bit)
{
    if (bit < 32) {
        if (Event_Mask & (1UL << bit)) {
            printf("HW-Event(%d)\n", bit);
            return 1; // Event is set
        }
    }

    return 0; // Event is not set
}

/**
 * @brief Clear a hardware event state bit
 * @param bit Event bit
 */
static void posix_hw_event_clear(uint8_t bit)
{
    if (bit < 32) {
        Event_Mask &= ~(1UL << bit);
        printf("HW-Event Cleared(%d)\n", bit);
    }
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
/* file name used for storing EEPROM data */
static const char *EEPROM_Filename = "EEPROM.bin";

/**
 * @brief Format the EEPROM
 */
static void eepromFormat(void)
{
    FILE *pFile = NULL;
    uint8_t buffer[256];
    unsigned i = 0;

    printf("EEPROM: creating file %s\n", EEPROM_Filename);
    pFile = fopen(EEPROM_Filename, "wb");
    if (!pFile) {
        perror("fopen error");
        return;
    }
    for (i = 0; i < 256; i++) {
        buffer[i] = 0xFF;
    }
    for (i = 0; i < 256; i++) {
        fwrite(buffer, 1, sizeof(buffer), pFile);
    }
    fclose(pFile);
}

/**
 * @brief Read some data from the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to store
 * @param length number of bytes of data to read
 */
static size_t
eepromRead(uint16_t start_address, uint8_t *buffer, uint16_t length)
{
    size_t bytes_read = 0, bytes_to_read = 0;
    FILE *pFile = NULL;
    long offset = 0;
    int seeking = 0;

    pFile = fopen(EEPROM_Filename, "rb");
    if (!pFile) {
        eepromFormat();
        pFile = fopen(EEPROM_Filename, "rb");
    }
    if (pFile) {
        offset = start_address;
        seeking = fseek(pFile, offset, SEEK_SET);
        if (seeking == 0) {
            bytes_to_read = length;
            bytes_read = fread(buffer, 1, bytes_to_read, pFile);
            if (bytes_read != bytes_to_read) {
                perror("fread error");
            }
            fclose(pFile);
        } else {
            perror("fseek error");
        }
    }

    return bytes_read;
}

/**
 * @brief Write some data to the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to send
 * @param length number of bytes of data
 */
static size_t
eepromWrite(uint16_t start_address, uint8_t *buffer, uint16_t length)
{
    size_t bytes_written = 0, bytes_to_write = 0;
    FILE *pFile = NULL;
    long offset = 0;
    int seeking = 0;

    pFile = fopen(EEPROM_Filename, "rb+");
    if (pFile) {
        offset = start_address;
        seeking = fseek(pFile, offset, SEEK_SET);
        if (seeking == 0) {
            bytes_to_write = length;
            bytes_written = fwrite(buffer, 1, bytes_to_write, pFile);
            if (bytes_written != bytes_to_write) {
                perror("fwrite error");
            }
            fclose(pFile);
        }
    }

    return bytes_written;
}

#define UBASIC_FLASH_PAGE_SIZE 256

/**
 * @brief Write a variable to the EEPROM
 * @param Name Variable name
 * @param Vartype Variable type
 * @param datalen_bytes Data length in bytes
 * @param dataptr Pointer to the data
 */
static void posix_flash_write(
    uint8_t Name, uint8_t vartype, uint8_t datalen_bytes, uint8_t *dataptr)
{
    // Calculate the starting address based on variable name
    uint16_t start_address = Name * UBASIC_FLASH_PAGE_SIZE;
    uint8_t buffer[UBASIC_FLASH_PAGE_SIZE];
    size_t bytes_written = 0;

    // Prepare the buffer with the variable type and data length
    buffer[0] = vartype; // First byte is the variable type
    buffer[1] = datalen_bytes; // Second byte is the data length
    for (uint8_t i = 0; i < datalen_bytes; i++) {
        buffer[i + 2] = dataptr[i]; // Copy the actual data into the buffer
    }

    // Write the buffer to EEPROM
    eepromWrite(start_address, buffer, datalen_bytes + 2);
    if (bytes_written != datalen_bytes + 2) {
        perror("flash_write error");
    }
}

/**
 * @brief Read a variable from the EEPROM
 * @param Name Variable name
 * @param Vartype Variable type
 * @param dataptr Pointer to store the data
 * @param datalen Pointer to store the data length
 */
static void posix_flash_read(
    uint8_t Name, uint8_t vartype, uint8_t *dataptr, uint8_t *datalen)
{
    // Calculate the starting address based on variable name
    uint16_t start_address = Name * UBASIC_FLASH_PAGE_SIZE;
    uint8_t buffer[UBASIC_FLASH_PAGE_SIZE];
    size_t bytes_read = 0;

    // Read the data from EEPROM
    bytes_read = eepromRead(start_address, buffer, UBASIC_FLASH_PAGE_SIZE);
    if (bytes_read != UBASIC_FLASH_PAGE_SIZE) {
        perror("flash_read error");
    }

    // Check if the variable type matches
    if (buffer[0] == vartype) {
        *datalen = buffer[1]; // Get the data length
        for (uint8_t i = 0; i < *datalen; i++) {
            dataptr[i] =
                buffer[i + 2]; // Copy the actual data into the provided pointer
        }
    } else {
        *datalen = 0; // If type does not match, set length to 0
    }
}
#endif

#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
/**
 * @brief Retrieves the system time, in milliseconds.
 * @return The system time, in milliseconds.
 */
static uint32_t posix_mstimer_now(void)
{
    struct timespec now;
    uint32_t ticks;
    static struct timespec start = { 0, 0 };
    static bool initialized = false;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (initialized) {
        ticks = (now.tv_sec - start.tv_sec) * 1000L +
            (now.tv_nsec - start.tv_nsec) / 1000000L;
    } else {
        start.tv_sec = now.tv_sec;
        start.tv_nsec = now.tv_nsec;
        ticks = 0;
        initialized = true;
    }

    return ticks;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
/**
 * @brief Generate a random number
 * @param size Size of the random number in bits
 * @return Random number size-bits wide
 */
static uint32_t posix_random_uint32(uint8_t size)
{
    uint32_t value = 0, grains = 0;
    uint8_t k, i;
    static bool initialized = false;

    if (!initialized) {
        srand(0);
        initialized = true;
    }
    for (k = 0; k < 4; k++) {
        grains = 0;
        for (i = 0; i < (size >> 1); i++) {
            /* Two LS bits are most likely most random */
            grains |= (rand() & 0x00000003) << (2 * i);
        }
        value ^= grains;
    }

    return value;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
static int16_t dutycycle_pwm_ch[UBASIC_SCRIPT_HAVE_PWM_CHANNELS];

/**
 * @brief Configure the PWM
 * @param psc Prescaler
 * @param per Period
 */
static void posix_pwm_config(uint16_t psc, uint16_t per)
{
    printf("pwm_config(%d, %d)\n", psc, per);
}

/**
 * @brief Write a value to the PWM
 * @param ch Channel
 * @param dutycycle Duty cycle
 */
static void posix_pwm_write(uint8_t ch, int16_t dutycycle)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        dutycycle_pwm_ch[ch] = dutycycle;
    }
}

/**
 * @brief Read a value from the PWM
 * @param ch Channel
 * @return Duty cycle
 */
static int16_t posix_pwm_read(uint8_t ch)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        return dutycycle_pwm_ch[ch];
    }
    return 0;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
/**
 * @brief Configure the ADC
 * @param sampletime Sample time
 * @param nreads Number of reads
 */
static void posix_adc_config(uint8_t sampletime, uint8_t nreads)
{
    printf("adc_config(%d, %d)\n", sampletime, nreads);
}

/**
 * @brief Read a value from the ADC
 * @param channel Channel
 * @return ADC value
 */
static int16_t posix_adc_read(uint8_t channel)
{
    printf("adc_read(%d)\n", channel);
    return (int16_t)posix_random_uint32(12);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
/**
 * @brief Configure the GPIO
 * @param ch Channel
 * @param mode Mode
 * @param freq Frequency
 */
static void posix_gpio_config(uint8_t ch, int8_t mode, uint8_t freq)
{
    printf("gpio_config(%d, %d, %d)\n", ch, mode, freq);
}

/**
 * @brief Write a value to the GPIO
 * @param ch Channel
 * @param pin_state Pin state
 */
static void posix_gpio_write(uint8_t ch, uint8_t pin_state)
{
    printf("gpio_write(%d, %d)\n", ch, pin_state);
}

/**
 * @brief Read a value from the GPIO
 * @param ch Channel
 * @return GPIO value
 */
static int8_t posix_gpio_read(uint8_t ch)
{
    printf("gpio_read(%d)\n", ch);
    return 0;
}
#endif

/**
 * @brief Initialize the hardware drivers
 * @param data Pointer to the ubasic data structure
 */
void ubasic_hardware_init(struct ubasic_data *data)
{
#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
    data->mstimer_now = posix_mstimer_now;
#endif
    data->flash_write = posix_flash_write;
    data->flash_read = posix_flash_read;
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    data->hw_event = posix_hw_event;
    data->hw_event_clear = posix_hw_event_clear;
#endif
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    data->pwm_config = posix_pwm_config;
    data->pwm_write = posix_pwm_write;
    data->pwm_read = posix_pwm_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    data->adc_config = posix_adc_config;
    data->adc_read = posix_adc_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    data->gpio_config = posix_gpio_config;
    data->gpio_write = posix_gpio_write;
    data->gpio_read = posix_gpio_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    data->random_uint32 = posix_random_uint32;
#endif
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    data->serial_write = posix_serial_write;
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    data->ubasic_getc = posix_serial_getc;
#endif
}
