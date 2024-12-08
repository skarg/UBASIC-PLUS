#ifndef POSIX_SERIAL_H
#define POSIX_SERIAL_H
#include <stdint.h>

void print_serial(char * msg);
void print_serial_n(char * msg, uint16_t n);
void print_numbered_lines(char * script);

uint8_t serial_input_available(void);
uint8_t serial_input (char * buffer, uint8_t len);

#endif