#ifndef POSIX_DIGIO_H
#define POSIX_DIGIO_H
#include <stdint.h>

void pinMode(uint8_t ch, int8_t mode, uint8_t freq);
int8_t digitalWrite(uint8_t ch, uint8_t PinState);
int8_t digitalRead(uint8_t ch);

#endif
