#ifndef __UBASIC_CLI_H__
#define __UBASIC_CLI_H__

#include "ubasic.h"

#define UBASIC_CLI_INIT 0
#define UBASIC_CLI_IDLE 1
#define UBASIC_CLI_LOADED 2
#define UBASIC_CLI_RUNNING 3
#define UBASIC_CLI_PROG 4

#define UBASIC_SCRIPT_SIZE_MAX (1024)

const char *ubasic_cli_flash_vartype_text(uint8_t vartype);
void ubasic_cli(struct ubasic_data *data);

#endif
