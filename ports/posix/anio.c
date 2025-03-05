#include <stdint.h>
#include <stdio.h>
#include "ubasic/ubasic.h"

#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
static int16_t dutycycle_pwm_ch[UBASIC_SCRIPT_HAVE_PWM_CHANNELS];
#endif

void ubasic_pwm_config(uint16_t psc, uint16_t per)
{
    printf("Analog_Output_Config(%d, %d)\n", psc, per);
}

void ubasic_pwm_write(uint8_t ch, int16_t dutycycle)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        dutycycle_pwm_ch[ch] = dutycycle;
    }
}

int16_t ubasic_pwm_read(uint8_t ch)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        return dutycycle_pwm_ch[ch];
    }
    return 0;
}

void Analog_Input_Config(uint8_t sampletime, uint8_t nreads)
{
    printf("Analog_Input_Config(%d, %d)\n", sampletime, nreads);
}

int16_t Analog_Input_Read(uint8_t channel)
{
    printf("Analog_Input_Read(%d)\n", channel);
    return (int16_t)RandomUInt32(12);
}
