#include <stdarg.h>
#include <stdio.h>
#include "cli.h"
#include "ubasic.h"

static uint8_t serial_getline_poll(struct ubasic_data *data)
{
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    if (data->serial_getline_poll) {
        return data->serial_getline_poll();
    }
#endif
    return 0;
}

static uint8_t serial_read(struct ubasic_data *data, char *buffer, uint8_t len)
{
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    if (data->serial_read) {
        return data->serial_read(buffer, len);
    }
#endif
    return 0;
}

/*---------------------------------------------------------------------------*/
static void
serial_write(struct ubasic_data *data, const char *buffer, uint16_t n)
{
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    if (data->serial_write) {
        data->serial_write(buffer, n);
    }
#endif
}

static void serial_write_string(struct ubasic_data *data, const char *msg)
{
    serial_write(data, msg, strlen(msg));
}

static void flash_read(
    struct ubasic_data *data,
    uint8_t Name,
    uint8_t Vartype,
    uint8_t *dataptr,
    uint8_t *datalen)
{
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    if (data->flash_read) {
        data->flash_read(Name, Vartype, dataptr, datalen);
    }
#endif
}

static void print_numbered_lines(struct ubasic_data *data, const char *script)
{
    uint16_t counter = 0;
    char msg[32];

    const char *c = script, *d = 0, *e = 0;

    do {
        counter++;
        sprintf(msg, "%02u ", counter);
        serial_write_string(data, msg);

        const char *s = c;
        while (*s == ' ') {
            ++s;
        }

        /* important: because two EOLs are used make sure that the first of the
         * two is selected ! */
        d = strchr(s, ';');
        e = strchr(s, '\n');
        if (e) {
            d = d - e > 0 ? e : d;
        }

        if (d) {
            serial_write(data, s, d - s);
            c = d + 1;
        } else {
            serial_write_string(data, s);
        }

        serial_write_string(data, "\n");
    } while (d);
}

/* Example Scripts for demo command
 * ---------------------------------------------------------*/
static const char welcome_msg[] = "\
Welcome to uBasic-Plus by M.Kostrun.\n\
Expands upon uBasic by A.Dunkels,\n\
uBasic with string by D.Mitchell,\n\
and uBasic for CHDK by P.d'Angelo.\n";

#if defined(UBASIC_SCRIPT_HAVE_DEMO_SCRIPTS)
static const char *program[] = {

    "\
println 'Demo 1 - Warm-up';\
gosub l1;\
for i = 1 to 2;\
for j = 1 to 2;\
println 'i,j=',i,j;\
next j;\
next i;\
println 'Demo 1 Completed';\
end;\
:l1 \
println 'subroutine';\
return;",

    "\
println 'Demo 2 - ubasic with strings';\
a$='abcdefghi';\
b$='123456789';\
println 'a$=' a$;\
println 'b$=' b$;\
println 'Length of a$=', len(a$);\
println 'Length of b$=', len(b$);\
if (len(a$) == len(b$)) then println 'same length';\
if (a$ == b$) then println 'same string';\
c$=left$(a$+b$,12);\
println c$;\
c$=right$(a$+b$, 12);\
println c$;\
c$=mid$(a$+b$, 8,8);\
println c$;\
c$=str$(13+42);\
println c$;\
println len(c$);\
println len('this' + 'that');\
c$ = chr$(34);\
println 'c$=' c$;\
j = asc(c$);\
println 'j=' j;\
println val('12345');\
i=instr(3, '123456789', '67');\
println 'position of 67 in 123456789 is', i;\
println mid$(a$,2,2)+'xyx';\
println 'Demo 2 Completed';",

    "\
println 'Demo 3 - Plus';\
tic(1);\
for i = 1 to 2;\
  j = i + 0.25 + 1/2;\
  println 'j=' j;\
  k = sqrt(2*j) + ln(4*i) + cos(i+j) + sin(j);\
  println 'k=' k;\
next i;\
println 'math duration=' toc(1);\
tic(2);\
sleep(0.3);\
println 'sleep(0.3)=' toc(2);\
for i = 1 to 2;\
println 'ran(' i ')=' ran;\
next i;\
for i = 1 to 2;\
println 'uniform(' i ')=' uniform;\
next i;\
for i = 1 to 2;\
x = 10 * uniform;\
println 'x=' x;\
println 'floor(x)=' floor(x);\
println 'ceil(x)=' ceil(x);\
println 'round(x)=' round(x);\
println 'x^3=' pow(x,3);\
next i;\
println 'Digital Write Test';\
pinmode(0xc0,-1,0);\
pinmode(0xc1,-1,0);\
pinmode(0xc2,-1,0);\
pinmode(0xc3,-1,0);\
for j = 0 to 2;\
  dwrite(0xc0,(j % 2));\
  dwrite(0xc1,(j % 2));\
  dwrite(0xc2,(j % 2));\
  dwrite(0xc3,(j % 2));\
  sleep(0.5);\
next j;\
println 'Press the Blue Button or type kill!';\
:presswait \
  if (flag(1)==0) then goto presswait;\
tic(1);\
println 'Blue Button pressed!';\
:deprwait \
  if (flag(2)==0) then goto deprwait;\
println 'duration =' toc(1);\
println 'Blue Button de-pressed!';\
println 'Demo 3 Completed';\
end;",

    "\
println 'Demo 4 - Input 5x with 5s timeouts';\
tic(1);\
dim a@(5);\
for i = 1 to 5;\
  print '?';\
  input a@(i),5000;\
next i;\
println 'end of input';\
for i = 1 to 5;\
  println 'a(' i ') = ' a@(i);\
next i;\
println 'duration=' toc(1);\
println 'Sleeping for 0.5s';\
tic(2);\
sleep(0.5);\
println 'sleep(0.5)=' toc(2);\
println 'Demo 4 Completed';\
end;",

    "\
println 'Demo 5 - analog inputs and arrays';\
aread_conf(7,16);\
n = 0;\
m = 0;\
s = 5;\
for i = 1 to s;\
  x = aread(16);\
  y = aread(17);\
  println 'VREF,TEMP=', x, y;\
  n = avgw(x,n,s);\
  m = avgw(y,m,s);\
  println 'n=', n;\
next i;\
println 'average x y=', n, m;\
for i = 1 to 1;\
  n = floor(10 * uniform) + 2 ;\
  dim b@(n);\
  for j = 1 to n;\
    b@(j) = ran;\
    println 'b@(' j ')=' b@(j);\
  next j;\
next i;\
println 'Demo 5 Completed';\
end;",

    "\
println 'Demo 6: Multiline if, while';\
println 'Test If: 1';\
for i=1 to 10 step 0.125;\
  x = uniform;\
  if (x>=0.5) then;\
    println x, 'is greater then 0.5';\
  else;\
    println x, 'is smaller then 0.5';\
  endif;\
  println 'i=' i;\
next i;\
println 'End of If-test 1';\
println 'Test While: 1';\
i=10;\
while ((i>=0)&&(uniform<=0.9));\
  i = i - 0.125;\
  println 'i =', i;\
endwhile;\
println 'End of While-test 1';\
println 'Demo 6 Completed';\
end",

    "\
println 'Demo 7: Analog Read or Kill';\
y=0;\
:startover \
  x = aread(10);\
  if (abs(x-y)>20) then;\
    y = x;\
    println 'x=',x;\
  endif;\
  sleep (0.2);\
goto startover;\
end",

    "\
println 'Demo 8: analog write (PWM) 4-Channel Test';\
p = 65536;\
for k = 1 to 10;\
  p = p/2;\
  awrite_conf(p,4096);\
  println 'prescaler = ' p;\
  for i = 1 to 10;\
    for j = 1 to 4;\
      awrite(j,4095*uniform);\
    next j;\
    println '    analog write = ' awrite(1),awrite(2),awrite(3),awrite(4);\
    println '    sleep(0.5)';\
    sleep(0.5);\
  next i;\
next k;\
awrite(1,0);\
awrite(2,0);\
awrite(3,0);\
awrite(4,0);\
println 'Demo 8 Completed';\
end",

    "\
clear;\
println 'Demo 9: store/recall with FLASH';\
if (recall(x)==0) then;\
  println 'generating x';\
  x = uniform;\
  store(x);\
  println 'storing x=' x;\
endif;\
println 'recall: x=' x;\
if (recall(y@)==0) then;\
  println 'generating y@';\
  dim y@(10);\
  for i=1 to 10;\
    y@(i) = uniform;\
  next i;\
  store(y@);\
  println 'storing y@' = y@;\
endif;\
println 'recall: y@';\
for i=1 to 10;\
  println '  y@('i')=' y@(i);\
next i;\
if (recall(s$)==0) then;\
  println 'generating s';\
  s$='what is going on?';\
  store(s$);\
  println 'store: s$',s$;\
endif;\
println 'recall: s$',s$;\
println 'Demo 9 Completed';\
end"
};
#endif

/* Private variables ---------------------------------------------------------*/
static char script[UBASIC_SCRIPT_SIZE_MAX];
static char statement[UBASIC_STATEMENT_SIZE_MAX];
static uint8_t cli_state = UBASIC_CLI_INIT;

/**
 * @brief Print with a printf string
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 * PRINT_ENABLED is non-zero
 * @return number of characters printed
 */
static int serial_printf(struct ubasic_data *data, const char *format, ...)
{
    int length = 0;
    char buffer[256];
    va_list ap;

    va_start(ap, format);
    length = vsnprintf(buffer, sizeof(buffer), format, ap);
    serial_write_string(data, buffer);
    va_end(ap);

    return length;
}

const char *ubasic_cli_flash_vartype_text(uint8_t vartype)
{
    switch (vartype) {
        case UBASIC_RECALL_STORE_TYPE_VARIABLE:
            return "variable";
        case UBASIC_RECALL_STORE_TYPE_STRING:
            return "string";
        case UBASIC_RECALL_STORE_TYPE_ARRAY:
            return "array";
        default:
            return "unknown";
    }
}

static void ubasic_cli_flash_dump(struct ubasic_data *data)
{
    uint8_t name;
    uint8_t vartype;
    uint8_t datalen = 0;
    uint8_t buffer[256] = { 0 };

    for (name = 0; name < 255; name++) {
        for (vartype = 0; vartype < UBASIC_RECALL_STORE_TYPE_MAX; vartype++) {
            flash_read(data, name, vartype, buffer, &datalen);
            if (datalen > 0) {
                serial_printf(
                    data, "%s %c: Length=%d, Data=",
                    ubasic_cli_flash_vartype_text(vartype), name + 'a',
                    datalen);
                for (uint8_t i = 0; i < datalen; i++) {
                    serial_printf(data, "%02X ", buffer[i]);
                }
                serial_printf(data, "\n");
            }
        }
    }
}

void ubasic_cli(struct ubasic_data *data)
{
    if (cli_state == UBASIC_CLI_INIT) {
        serial_write_string(data, welcome_msg);
        serial_write_string(data, "\n>");
        cli_state = UBASIC_CLI_IDLE;
    }

    if ((cli_state == UBASIC_CLI_LOADED) || (cli_state == UBASIC_CLI_RUNNING)) {
        ubasic_run_program(data);
        cli_state = UBASIC_CLI_RUNNING;
        if (ubasic_finished(data)) {
            cli_state = UBASIC_CLI_IDLE;
            serial_write_string(data, "\n>");
        } else if (!ubasic_waiting_for_input(data)) {
            if (serial_getline_poll(data)) {
                if (serial_read(data, statement, sizeof(statement)) &&
                    strstr(statement, "kill")) {
                    // enter programming mode
                    serial_write_string(data, "killed\n>");
                    cli_state = UBASIC_CLI_IDLE;
                    ubasic_load_program(data, NULL);
                    return;
                }
            }
        }
    }

    if (cli_state != UBASIC_CLI_RUNNING) {
        if (serial_getline_poll(data) &&
            serial_read(data, statement, sizeof(statement))) {
            if (strstr(statement, "help")) {
                serial_write_string(
                    data, "Commands: help, run, cat, prog, save, edit");
#if defined(UBASIC_SCRIPT_HAVE_DEMO_SCRIPTS)
                serial_write_string(data, ", demo 1-9");
#endif
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
                serial_write_string(data, ", flash");
#endif
                serial_write_string(data, "\n>");
                return;
            } else if (strstr(statement, "prog")) {
                // enter programming mode
                script[0] = 0;
                serial_write_string(
                    data, "Enter your script. Type 'run' to execute!\n>");
                cli_state = UBASIC_CLI_PROG;
                return;
            } else if (strstr(statement, "run")) {
                // run script
                serial_write_string(data, "run\n");
                if (strlen(script) > 0) {
                    ubasic_load_program(data, script);
                    cli_state = UBASIC_CLI_LOADED;
                }
                return;
            } else if (strstr(statement, "cat")) {
                // list script
                serial_write_string(data, "cat\n");
                if (strlen(script) > 0) {
                    print_numbered_lines(data, script);
                    serial_write_string(data, "\n");
                }
                serial_write_string(data, ">");
                return;
            } else if (strstr(statement, "save")) {
                // save script: exit PROG mode
                serial_write_string(data, "save\n>");
                if (strlen(script) > 0) {
                    cli_state = UBASIC_CLI_IDLE;
                }
                return;
            } else if (strstr(statement, "edit")) {
                // edit script: re-enter PROG mode
                serial_write_string(data, "edit\n>");
                if (strlen(script) > 0) {
                    cli_state = UBASIC_CLI_PROG;
                }
                return;
            }
#if defined(UBASIC_SCRIPT_HAVE_DEMO_SCRIPTS)
            else if (strstr(statement, "demo")) {
                // run script
                serial_write_string(data, statement);
                serial_write_string(data, "\n");
                char *s = &statement[4];
                while (*s == ' ') {
                    ++s;
                }
                uint8_t idx = *s - '0';
                if (idx < 10) {
                    ubasic_load_program(data, program[idx - 1]);
                    cli_state = UBASIC_CLI_LOADED;
                } else {
                    serial_write_string(data, "demo script out of range!\n");
                }
                return;
            }
#endif
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
            else if (strstr(statement, "flash")) {
                // test write
                serial_write_string(data, "flash\n");
                ubasic_cli_flash_dump(data);
                serial_write_string(data, ">");
                return;
            }
#endif
            if (cli_state == UBASIC_CLI_PROG) {
                // add statement to the script
                // put ';' at the end of each new line
                if (strlen(script) > 0) {
                    while (script[strlen(script) - 1] == ' ' ||
                           script[strlen(script) - 1] == '\t') {
                        script[strlen(script) - 1] = '\0';
                    }
                    if (script[strlen(script) - 1] != '\n' &&
                        script[strlen(script) - 1] != ';') {
                        sprintf(&script[strlen(script)], "\n");
                    }
                }
                char *s = statement;
                while (*s == ' ') {
                    s++;
                }
                sprintf(&script[strlen(script)], "%s", s);
                serial_write_string(data, s);
                serial_write_string(data, "\n>");
            } else {
                // prepare statement for execution
                if (strlen(statement) > 0) {
                    serial_write_string(data, statement);
                    serial_write_string(data, "\n");
                    ubasic_load_program(data, statement);
                    cli_state = UBASIC_CLI_LOADED;
                }
            }
        }
    }

    return;
}
