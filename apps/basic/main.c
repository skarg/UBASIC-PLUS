
#include "ubasic/ubasic.h"
#include "ubasic/cli.h"

static struct ubasic_data UBasic_Program;

int main(void)
{
    UBasic_Program.mstimer_now = ubasic_mstimer_now;
    UBasic_Program.pwm_config = ubasic_pwm_config;
    UBasic_Program.pwm_write = ubasic_pwm_write;
    UBasic_Program.pwm_read = ubasic_pwm_read;

    printf("%s\n", cli_welcome_msg());
    while (1)
    {
      ubasic_cli(&UBasic_Program);
    }
}
