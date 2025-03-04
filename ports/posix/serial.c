#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#if defined(_MSC_VER)
#include <conio.h> /* for kbhit and getch */
#else
#include <termios.h> /* used in kbhit() */
#include <sys/ioctl.h> /* used in kbhit() */
#endif
#include "ubasic/ubasic.h"

static char Serial_Buffer[256];

#if defined(__GNUC__)
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

void print_serial(const char *msg)
{
    printf("%s", msg);
    fflush(stdout);
}

void print_serial_n(const char *msg, uint16_t n)
{
    printf("%.*s", n, msg);
}

/**
* @brief Non-blocking terminal task
* @param ch - character read from serial port
* @return 1 if line is complete and ready to process, 0 if not
*/
static uint8_t serial_input_handler(
    char *buffer,
    unsigned buffer_len,
    char ch)
{
    uint8_t done = 0;
    size_t i;

    if (!buffer || buffer_len == 0) {
        return 0;
    }
    switch (ch) {
        case '\a':
        case '\f':
        case '\t':
        case '\r':
        case '\v':
            /* ignored characters */
            break;
        case 0x1B:
            /* escape */
            /* clear buffer */
            buffer[0] = 0;
            done = 1;
            break;
        case '\b':
            /* backspace */
            /* erase current character */
            i = strlen(buffer);
            if ((i > 0) && (i < (buffer_len - 1))) {
                buffer[i - 1] = 0;
            }
            break;
        case '\n':
            /* enter */
            done = 1;
            break;
        default:
            /* all the rest of the characters */
            /* leave room for null at the end */
            i = strlen(buffer);
            if (i < (buffer_len - 1)) {
                buffer[i] = ch;
                buffer[i + 1] = 0;
            }
            break;
    }

    return done;
}

/**
 * @brief Gather key presses until new-line is recieved or buffer is full
 * @return 1 if buffer is full or new-line is received, 0 line is not complete
 */
uint8_t serial_input_available(void)
{
    char ch;

    if (kbhit()) {
        ch = getchar();
        return serial_input_handler(Serial_Buffer, sizeof(Serial_Buffer), ch);
    }

    return 0;
}

/**
 * @brief Copy the serial buffer to the provided buffer
 * @param buffer Pointer to the buffer to copy to
 * @param len Length of the buffer
 * @return Number of bytes copied
 * @note The serial buffer is cleared after copying
 */
uint8_t serial_input(char *buffer, uint8_t len)
{
    uint16_t i;

    if (!buffer || len == 0) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        buffer[i] = Serial_Buffer[i];
        if (Serial_Buffer[i] == '\0') {
            break;
        }
    }
    buffer[i] = '\0'; // Ensure null-termination
    Serial_Buffer[0] = '\0';

    return i;
}
