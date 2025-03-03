#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ubasic/ubasic.h"

void print_serial(const char *msg)
{
    printf("%s", msg);
}

void print_serial_n(const char *msg, uint16_t n)
{
    printf("%.*s", n, msg);
}

uint8_t serial_input_available(void)
{
    return 0;
}

uint8_t serial_input(char *buffer, uint8_t len)
{
    (void)buffer;
    (void)len;
    return 0;
}
