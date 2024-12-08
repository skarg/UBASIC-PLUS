#ifndef POSIX_ANALOG_IO_H
#define POSIX_ANALOG_IO_H
#include <stdint.h>

void pwm_Init(uint8_t ch);
void    analogWriteConfig(uint16_t psc, uint16_t per);
int16_t analogWrite(uint8_t ch, int16_t dutycycle);
void      analogReadConfig(uint8_t sampletime, uint8_t nreads);
int16_t   analogRead(uint8_t channel);

#endif
