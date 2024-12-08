#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "serial.h"

void print_serial(char * msg)
{
    printf("%s", msg);
}

void print_serial_n(char * msg, uint16_t n)
{
    printf("%.*s", n, msg);
}

uint8_t serial_input_available(void)
{
    return 0;
}

uint8_t serial_input (char * buffer, uint8_t len)
{
    return 0;
}
