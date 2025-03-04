#ifndef __RANDOM_H
#define __RANDOM_H

void MX_ADC_Init(void);
void MX_CRC_Init(void);
void      Analog_Input_Config(uint8_t sampletime, uint8_t nreads);
int16_t   Analog_Input_Read(uint8_t channel);
uint32_t  RandomUInt32(uint8_t size);

#endif
