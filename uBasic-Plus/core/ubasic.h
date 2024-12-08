/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Modified to support simple string variables and functions by David Mitchell
 * November 2008.
 * Changes and additions are marked 'string additions' throughout
 */

/*
 * Modified to support fixed point arithmetic, and number of math and io and
 * hardware functions by Marijan Kostrun, January-February 2018.
 * uBasic-Plus Copyright (c) 2017-2018, M. Kostrun
 */

#ifndef __UBASIC_H__
#define __UBASIC_H__

#include "config.h"
#include "tokenizer.h"

#define status_RUN   0x80
#define status_MASK_NOT_RUNNING 0x7f
#define status_IDLE  0x00

/* define a structure with bit fields */
typedef union
{
  uint8_t byte;
  struct
  {
    uint8_t notInitialized : 1;
    uint8_t stringstackModified : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t WaitForSerialInput : 1;
    uint8_t Error       : 1;
    uint8_t isRunning   : 1;
  } bit;
} UBASIC_STATUS;

#define MAX_FOR_STACK_DEPTH 4
struct ubasic_for_state
{
    uint16_t line_after_for;
    uint8_t for_variable;
    VARIABLE_TYPE to;
    VARIABLE_TYPE step;
};

#define MAX_WHILE_STACK_DEPTH 4
struct ubasic_while_state
{
    uint16_t line_while;
    int16_t line_after_endwhile;
};

#define MAX_GOSUB_STACK_DEPTH 10
#define MAX_IF_STACK_DEPTH 4

struct ubasic_data
{
    UBASIC_STATUS status;
    uint8_t input_how;
    struct tokenizer_data tree;

#if defined(VARIABLE_TYPE_ARRAY)
    VARIABLE_TYPE arrays_data[VARIABLE_TYPE_ARRAY];
    int16_t free_arrayptr;
    int16_t arrayvariable[MAX_VARNUM];
#endif
    char const *program_ptr;

    uint16_t gosub_stack[MAX_GOSUB_STACK_DEPTH];
    uint8_t gosub_stack_ptr;

    struct ubasic_for_state for_stack[MAX_FOR_STACK_DEPTH];
    uint8_t for_stack_ptr;

    int16_t if_stack[MAX_IF_STACK_DEPTH];
    uint8_t if_stack_ptr;

    struct ubasic_while_state while_stack[MAX_WHILE_STACK_DEPTH];
    uint8_t while_stack_ptr;

    VARIABLE_TYPE variables[MAX_VARNUM];

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    uint8_t varnum;
#endif

#if defined(VARIABLE_TYPE_STRING)
    char stringstack[MAX_BUFFERLEN];
    int16_t freebufptr;
    int16_t stringvariables[MAX_SVARNUM];
#endif

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    uint8_t input_varnum;
    uint8_t input_type;
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    VARIABLE_TYPE input_array_index;
#endif
    // What it means to support SLEEP:
    //    An interrupt routine has to exist which checks the value of
    //        'sleeping_ms'
    //    and decrease it by one every ms. As long as this is nonzero
    //        ubasic_run()
    //    will return immediately and not execute BASIC script.
    //    The sleep() sets this to requested value of ms to sleep.
    #if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    uint32_t sleeping_ms;
    #endif
    #if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    int16_t dutycycle_pwm_ch[UBASIC_SCRIPT_HAVE_PWM_CHANNELS];
    #endif
};

void ubasic_load_program(struct ubasic_data *data, const char *program);
void ubasic_clear_variables(struct ubasic_data *data);
void ubasic_run_program(struct ubasic_data *data);
uint8_t ubasic_execute_statement(struct ubasic_data *data, char *statement);
uint8_t ubasic_finished(struct ubasic_data *data);
uint8_t ubasic_waiting_for_input(struct ubasic_data *data);

VARIABLE_TYPE ubasic_get_variable(struct ubasic_data *data, uint8_t varnum);
void ubasic_set_variable(struct ubasic_data *data, uint8_t varum, VARIABLE_TYPE value);

#if defined(VARIABLE_TYPE_ARRAY)
void ubasic_dim_arrayvariable(struct ubasic_data *data, uint8_t varnum, int16_t size);
void ubasic_set_arrayvariable(struct ubasic_data *data, uint8_t varnum, uint16_t idx, VARIABLE_TYPE value);
VARIABLE_TYPE ubasic_get_arrayvariable(struct ubasic_data *data, uint8_t varnum, uint16_t idx);
#endif

#if defined(VARIABLE_TYPE_STRING)
int16_t ubasic_get_stringvariable(struct ubasic_data *data, uint8_t varnum);
void ubasic_set_stringvariable(struct ubasic_data *data, uint8_t varnum, int16_t size);
#endif

#endif /* __UBASIC_H__ */
