
#include "ubasic/ubasic.h"
#include "ubasic/cli.h"

static struct ubasic_data UBasic_Program;

int main(void)
{
    ubasic_hardware_init(&UBasic_Program);
    while (1) {
        ubasic_cli(&UBasic_Program);
    }
}
