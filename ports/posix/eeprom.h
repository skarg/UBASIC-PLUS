#ifndef POSIX_EEPROM_H
#define POSIX_EEPROM_H
#include <stdint.h>

void EE_Init(void);
void EE_WriteVariable(uint8_t Name, uint8_t Vartype, uint8_t datalen_bytes, uint8_t *dataptr);
void EE_ReadVariable(uint8_t Name, uint8_t Vartype, uint8_t *dataptr, uint8_t *datalen);
void EE_DumpFlash(void);
