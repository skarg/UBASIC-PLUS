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

/* Includes: Generic ----------------------------------------------------------*/
#include "config.h"
#include "ubasic.h"
#include "tokenizer.h"

#if defined(VARIABLE_TYPE_STRING)
static int16_t sexpr(struct ubasic_data *data);
static int16_t scpy(struct ubasic_data *data, char *);
static int16_t sconcat(struct ubasic_data *data, char *, char *);
static int16_t sleft(struct ubasic_data *data, char *, int16_t);
static int16_t sright(struct ubasic_data *data, char *, int16_t);
static int16_t smid(struct ubasic_data *data, char *, int16_t, int16_t);
static int16_t sstr(struct ubasic_data *data, VARIABLE_TYPE j);
static int16_t schr(struct ubasic_data *data, VARIABLE_TYPE j);
static uint8_t sinstr(uint16_t, char *, char *);
static uint8_t strvar(struct ubasic_data *data, int16_t size)
{
  return ((uint8_t) * ((data->stringstack) + (size)));
}
static char *strptr(struct ubasic_data *data, int16_t size)
{
  return ((char *)((data->stringstack) + (size) + 1));
}
#endif
static VARIABLE_TYPE relation(struct ubasic_data *data);
static void numbered_line_statement(struct ubasic_data *data);
static void statement(struct ubasic_data *data);
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
static VARIABLE_TYPE recall_statement(struct ubasic_data *data);
#endif

/*---------------------------------------------------------------------------*/
void ubasic_clear_variables(struct ubasic_data *data)
{
  int16_t i;
  for (i = 0; i < MAX_VARNUM; i++)
  {
    data->variables[i] = 0;
#if defined(VARIABLE_TYPE_ARRAY)
    data->arrayvariable[i] = -1;
#endif
  }

#if defined(VARIABLE_TYPE_ARRAY)
  data->free_arrayptr = 0;
  for (i = 0; i < VARIABLE_TYPE_ARRAY; i++)
  {
    data->arrays_data[i] = 0;
  }
#endif

#if defined(VARIABLE_TYPE_STRING)
  data->freebufptr = 0;
  for (i = 0; i < MAX_SVARNUM; i++)
    data->stringvariables[i] = -1;
#endif
}

/*---------------------------------------------------------------------------*/
void ubasic_load_program(struct ubasic_data *data, const char *program)
{
  data->for_stack_ptr = data->gosub_stack_ptr = 0;
  if (data->status.bit.notInitialized)
  {
    ubasic_clear_variables(data);
  }
  data->status.byte = 0x00;
  if (program)
  {
    data->program_ptr = program;
    tokenizer_init(&data->tree, program);
    data->status.bit.isRunning = 1;
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t accept(struct tokenizer_data *tree, VARIABLE_TYPE token)
{

  if (token != tokenizer_token(tree))
  {
    tokenizer_error_print(tree, token);
    return 1;
  }
  {
    tokenizer_error_print(tree, token);
    return 1;
  }

  tokenizer_next(tree);
  return 0;
}

/*---------------------------------------------------------------------------*/
static void accept_cr(struct tokenizer_data *tree)
{
  while ((tokenizer_token(tree) != TOKENIZER_EOL) &&
         (tokenizer_token(tree) != TOKENIZER_ERROR) &&
         (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT))
  {
    tokenizer_next(tree);
  }

  if (tokenizer_token(tree) == TOKENIZER_EOL)
  {
    tokenizer_next(tree);
  }
}

#if defined(VARIABLE_TYPE_STRING)

/*---------------------------------------------------------------------------*/
uint8_t string_space_check(struct ubasic_data *data, uint16_t l)
{
  // returns true if not enough room for new string
  uint8_t i;
  i = ((MAX_BUFFERLEN - data->freebufptr) <= (l + 2)); // +2 to play it safe
  if (i)
  {
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
  }
  return i;
}

/*---------------------------------------------------------------------------*/
void clear_stringstack(struct ubasic_data *data)
{
  // if (!status.bit.stringstackModified )
  // return;

  data->status.bit.stringstackModified = 0;

  int16_t bottom = 0;
  int16_t len = 0;

  // find bottom of the stringstack skip allocated stringstack space
  while (*(data->stringstack + bottom) != 0)
  {
    bottom += strlen(strptr(data, bottom)) + 2;
    if (data->freebufptr == bottom)
    {
      return;
    }
  }

  int16_t top = bottom;
  do
  {
    len = strlen(strptr(data, top)) + 2;

    if (*(data->stringstack + top) > 0)
    {
      // moving stuff down
      for (uint8_t i = 0; i <= len; i++)
        *(data->stringstack + bottom + i) = *(data->stringstack + top + i);

      // update variable reference from top to bottom
      data->stringvariables[*(data->stringstack + bottom) - 1] = bottom;

      bottom += len;
    }
    top += len;
  } while (top < data->freebufptr);
  data->freebufptr = bottom;

  return;
}
/*---------------------------------------------------------------------------*/
// copy s1 at the end of stringstack and add a header
static int16_t scpy(struct ubasic_data *data, char *s1)
{
  if (!s1)
    return (-1);

  int16_t bp = data->freebufptr;
  int16_t l = strlen(s1);

  if (!l)
    return (-1);

  if (string_space_check(data, l))
    return (-1);

  data->status.bit.stringstackModified = 1;

  *(data->stringstack + bp) = 0;
  memcpy(data->stringstack + bp + 1, s1, l + 1);

  data->freebufptr = bp + l + 2;

  return bp;
}

/*---------------------------------------------------------------------------*/
// return the concatenation of s1 and s2 in a string at the end
// of the stringbuffer
static int16_t sconcat(struct ubasic_data *data, char *s1, char *s2)
{
  int16_t l1 = strlen(s1), l2 = strlen(s2);

  if (string_space_check(data, l1 + l2))
    return (-1);

  int16_t rp = scpy(data, s1);
  data->freebufptr -= 2; // last char in s1, will be overwritten by s2 header
  int16_t fp = data->freebufptr;
  char dummy = *(data->stringstack + fp);
  scpy(data, s2);
  *(data->stringstack + fp) = dummy; // overwrite s2 header
  return (rp);
}
/*---------------------------------------------------------------------------*/
static int16_t sleft(struct ubasic_data *data, char *s1, int16_t l) // return the left l chars of s1
{
  int16_t bp = data->freebufptr;
  int16_t rp = bp;

  if (l < 1)
    return (-1);

  if (string_space_check(data, l))
    return (-1);

  data->status.bit.stringstackModified = 1;

  if (strlen(s1) <= l)
  {
    return scpy(data, s1);
  }
  else
  {
    // write header
    *(data->stringstack + bp) = 0;
    bp++;
    memcpy(data->stringstack + bp, s1, l);
    bp += l;
    *(data->stringstack + bp) = 0;
    data->freebufptr = bp + 1;
  }

  return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t sright(struct ubasic_data *data, char *s1, int16_t l) // return the right l chars of s1
{
  int16_t j = strlen(s1);

  if (l < 1)
    return (-1);

  if (j <= l)
    l = j;

  if (string_space_check(data, l))
    return (-1);

  return scpy(data, s1 + j - l);
}

/*---------------------------------------------------------------------------*/
static int16_t smid(struct ubasic_data *data, char *s1, int16_t l1, int16_t l2) // return the l2 chars of s1 starting at offset l1
{
  int16_t bp = data->freebufptr;
  int16_t rp = bp;
  int16_t i, j = strlen(s1);

  if (l2 < 1 || l1 > j)
    return (-1);

  if (string_space_check(data, l2))
    return (-1);

  if (l2 > j - l1)
    l2 = j - l1;

  data->status.bit.stringstackModified = 1;

  *(data->stringstack + bp) = 0;
  bp++;

  for (i = l1; i < l1 + l2; i++)
  {
    *(data->stringstack + bp) = *(s1 + l1 - 1);
    bp++;
    s1++;
  }

  *(data->stringstack + bp) = '\0';
  data->freebufptr = bp + 1;
  return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t sstr(struct ubasic_data *data, VARIABLE_TYPE j) // return the integer j as a string
{
  int16_t bp = data->freebufptr;
  int16_t rp = bp;

  if (string_space_check(data, 10))
    return (-1);

  data->status.bit.stringstackModified = 1;

  *(data->stringstack + bp) = 0;
  bp++;

  sprintf((char *)(data->stringstack + bp), "%ld", j);

  data->freebufptr = bp + strlen((char *)(data->stringstack + bp)) + 1;

  return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t schr(struct ubasic_data *data, VARIABLE_TYPE j) // return the character whose ASCII code is j
{
  int16_t bp = data->freebufptr;
  int16_t rp = bp;

  if (string_space_check(data, 1))
    return (-1);

  data->status.bit.stringstackModified = 1;

  *(data->stringstack + bp) = 0;
  bp++;

  *(data->stringstack + bp) = j;
  bp++;

  *(data->stringstack + bp) = 0;
  bp++;

  data->freebufptr = bp;
  return rp;
}
/*---------------------------------------------------------------------------*/
static uint8_t sinstr(uint16_t j, char *s, char *s1) // return the position of s1 in s (or 0)
{
  char *p;
  p = strstr(s + j, s1);
  if (p == NULL)
    return 0;
  return (p - s + 1);
}

/*---------------------------------------------------------------------------*/
int16_t sfactor(struct ubasic_data *data)
{
  // string form of factor
  int16_t r = 0, s = 0;
  char tmpstring[MAX_STRINGLEN];
  VARIABLE_TYPE i, j;
  struct tokenizer_data *tree = &data->tree;

  switch (tokenizer_token(tree))
  {
  case TOKENIZER_LEFTPAREN:
    accept(tree, TOKENIZER_LEFTPAREN);
    r = sexpr(data);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_STRING:
    tokenizer_string(tree, tmpstring, MAX_STRINGLEN);
    r = scpy(data, tmpstring);
    accept(tree, TOKENIZER_STRING);
    break;

  case TOKENIZER_LEFT$:
    accept(tree, TOKENIZER_LEFT$);
    accept(tree, TOKENIZER_LEFTPAREN);
    s = sexpr(data);
    accept(tree, TOKENIZER_COMMA);
    i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    i = fixedpt_toint(i);
#endif
    r = sleft(data, strptr(data, s), i);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_RIGHT$:
    accept(tree, TOKENIZER_RIGHT$);
    accept(tree, TOKENIZER_LEFTPAREN);
    s = sexpr(data);
    accept(tree, TOKENIZER_COMMA);
    i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    i = fixedpt_toint(i);
#endif
    r = sright(data, strptr(data, s), i);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_MID$:
    accept(tree, TOKENIZER_MID$);
    accept(tree, TOKENIZER_LEFTPAREN);
    s = sexpr(data);
    accept(tree, TOKENIZER_COMMA);
    i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    i = fixedpt_toint(i);
#endif
    if (tokenizer_token(tree) == TOKENIZER_COMMA)
    {
      accept(tree, TOKENIZER_COMMA);
      j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
      j = fixedpt_toint(j);
#endif
    }
    else
    {
      j = 999; // ensure we get all of it
    }
    r = smid(data, strptr(data, s), i, j);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_STR$:
    accept(tree, TOKENIZER_STR$);
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    r = sstr(data, j);
    break;

  case TOKENIZER_CHR$:
    accept(tree, TOKENIZER_CHR$);
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    if (j < 0 || j > 255)
      j = 0;
    r = schr(data, j);
    break;

  default:
    r = ubasic_get_stringvariable(data, tokenizer_variable_num(tree));
    accept(tree, TOKENIZER_STRINGVARIABLE);
  }

  return r;
}

/*---------------------------------------------------------------------------*/

static int16_t sexpr(struct ubasic_data *data) // string form of expr
{
  int16_t s1, s2;
  s1 = sfactor(data);
  uint8_t op = tokenizer_token(&data->tree);
  while (op == TOKENIZER_PLUS)
  {
    tokenizer_next(&data->tree);
    s2 = sfactor(data);
    s1 = sconcat(data, strptr(data, s1), strptr(data, s2));
    op = tokenizer_token(&data->tree);
  }
  return s1;
}
/*---------------------------------------------------------------------------*/
static uint8_t slogexpr(struct ubasic_data *data) // string logical expression
{
  int16_t s1, s2;
  uint8_t r = 0;
  s1 = sexpr(data);
  uint8_t op = tokenizer_token(&data->tree);
  tokenizer_next(&data->tree);
  if (op == TOKENIZER_EQ)
  {
    s2 = sexpr(data);
    r = (strcmp(strptr(data, s1), strptr(data, s2)) == 0);
  }
  return r;
}
// end of string additions
#endif

/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE varfactor(struct ubasic_data *data)
{
  VARIABLE_TYPE r;
  struct tokenizer_data *tree = &data->tree;

  r = ubasic_get_variable(data, tokenizer_variable_num(tree));
  accept(tree, TOKENIZER_VARIABLE);
  return r;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE factor(struct ubasic_data *data)
{
  VARIABLE_TYPE r;
  VARIABLE_TYPE i, j;
#if defined(VARIABLE_TYPE_ARRAY)
  uint8_t varnum;
#endif
#if defined(VARIABLE_TYPE_STRING)
  int16_t s, s1;
#endif
  struct tokenizer_data *tree = &data->tree;

  switch (tokenizer_token(tree))
  {
#if defined(VARIABLE_TYPE_STRING)
  case TOKENIZER_LEN:
    accept(tree, TOKENIZER_LEN);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(strlen(strptr(data, sexpr(data))));
#else
    r = strlen(strptr(data, sexpr(data)));
#endif
    break;

  case TOKENIZER_VAL:
    accept(tree, TOKENIZER_VAL);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    s1 = sexpr(data);
    r = str_fixedpt(strptr(data, s1), strlen(strptr(data, s1)), 3);
#else
    r = atoi(strptr(sexpr()));
#endif
    break;

  case TOKENIZER_ASC:
    accept(tree, TOKENIZER_ASC);
    s = sexpr(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(*strptr(data, s));
#else
    r = *strptr(data, s);
#endif
    break;

  case TOKENIZER_INSTR:
    accept(tree, TOKENIZER_INSTR);
    accept(tree, TOKENIZER_LEFTPAREN);
    j = 1;
    if (tokenizer_token(tree) == TOKENIZER_NUMBER)
    {
      j = tokenizer_num(tree);
      accept(tree, TOKENIZER_NUMBER);
      accept(tree, TOKENIZER_COMMA);
    }
    if (j < 1)
      return 0;
    s = sexpr(data);
    accept(tree, TOKENIZER_COMMA);
    s1 = sexpr(data);
    accept(tree, TOKENIZER_RIGHTPAREN);
    r = sinstr(j, strptr(data, s), strptr(data, s1));
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
#endif
    break;
#endif

  case TOKENIZER_MINUS:
    accept(tree, TOKENIZER_MINUS);
    r = -factor(data);
    break;

  case TOKENIZER_LNOT:
    accept(tree, TOKENIZER_LNOT);
    r = !relation(data);
    break;

  case TOKENIZER_NOT:
    accept(tree, TOKENIZER_LNOT);
    r = ~relation(data);
    break;

#if defined(UBASIC_SCRIPT_HAVE_TICTOC)
  case TOKENIZER_TOC:
    accept(tree, TOKENIZER_TOC);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    if (r == 2)
      r = ubasic_script_tic1_ms;
    else if (r == 3)
      r = ubasic_script_tic2_ms;
    else if (r == 4)
      r = ubasic_script_tic3_ms;
    else if (r == 5)
      r = ubasic_script_tic4_ms;
    else if (r == 6)
      r = ubasic_script_tic5_ms;
    else
      r = ubasic_script_tic0_ms;
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
    accept(tree, TOKENIZER_RIGHTPAREN);
#endif
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
  case TOKENIZER_HWE:
    accept(tree, TOKENIZER_HWE);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    if (r)
    {
      if (hw_event & (1 << (r - 1)))
      {
        hw_event -= 0x01 << (r - 1);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        r = FIXEDPT_ONE;
#endif
      }
      else
        r = 0;
    }
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;
#endif /* #if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS) */

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
  case TOKENIZER_RAN:
    accept(tree, TOKENIZER_RAN);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = RandomUInt32(FIXEDPT_WBITS);
    r = fixedpt_fromint(r);
#else
    r = RandomUInt32(32);
#endif
    if (r < 0)
      r = -r;
    break;
#endif

  case TOKENIZER_ABS:
    accept(tree, TOKENIZER_ABS);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    if (r < 0)
      r = -r;
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  case TOKENIZER_POWER:
    accept(tree, TOKENIZER_POWER);
    accept(tree, TOKENIZER_LEFTPAREN);
    // argument:
    i = relation(data);
    accept(tree, TOKENIZER_COMMA);
    // exponent
    j = relation(data);
    r = fixedpt_pow(i, j);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_FLOAT:
    r = tokenizer_float(tree); /* 24.8 decimal number */
    accept(tree, TOKENIZER_FLOAT);
    break;

  case TOKENIZER_SQRT:
    accept(tree, TOKENIZER_SQRT);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_sqrt(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_SIN:
    accept(tree, TOKENIZER_SIN);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_sin(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_COS:
    accept(tree, TOKENIZER_COS);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_cos(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_TAN:
    accept(tree, TOKENIZER_TAN);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_tan(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_EXP:
    accept(tree, TOKENIZER_EXP);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_exp(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_LN:
    accept(tree, TOKENIZER_LN);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = fixedpt_ln(relation(data));
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
  case TOKENIZER_UNIFORM:
    accept(tree, TOKENIZER_UNIFORM);
    r = RandomUInt32(FIXEDPT_FBITS) & FIXEDPT_FMASK;
    break;
#endif

  case TOKENIZER_FLOOR:
    accept(tree, TOKENIZER_FLOOR);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    if (r >= 0)
    {
      r = r & (~FIXEDPT_FMASK);
    }
    else
    {
      uint32_t f = (r & FIXEDPT_FMASK);
      r = r & (~FIXEDPT_FMASK);
      if (f > 0)
        r -= FIXEDPT_ONE;
    }
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_CEIL:
    accept(tree, TOKENIZER_CEIL);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    if (r >= 0)
    {
      uint32_t f = (r & FIXEDPT_FMASK);
      r = r & (~FIXEDPT_FMASK);
      if (f > 0)
        r += FIXEDPT_ONE;
    }
    else
    {
      r = r & (~FIXEDPT_FMASK);
    }
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

  case TOKENIZER_ROUND:
    accept(tree, TOKENIZER_ROUND);
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    uint32_t f = (r & FIXEDPT_FMASK);
    if (r >= 0)
    {
      r = r & (~FIXEDPT_FMASK);
      if (f >= FIXEDPT_ONE_HALF)
        r += FIXEDPT_ONE;
    }
    else
    {
      r = r & (~FIXEDPT_FMASK);
      if (f <= FIXEDPT_ONE_HALF)
        r -= FIXEDPT_ONE;
    }
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;
#endif /* #if defined(VARIABLE_TYPE_FLOAT_AS ... */

  case TOKENIZER_INT:
    r = tokenizer_int(tree);
    accept(tree, TOKENIZER_INT);
    break;

  case TOKENIZER_NUMBER:
    r = tokenizer_num(tree);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
#endif
    accept(tree, TOKENIZER_NUMBER);
    break;

#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
  case TOKENIZER_PWM:
    accept(tree, TOKENIZER_PWM);
    accept(tree, TOKENIZER_LEFTPAREN);
    // single argument: channel
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    if (j < 1 || j > UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    {
      r = -1;
    }
    else
    {
      r = data->dutycycle_pwm_ch[j - 1];
    }
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
#endif
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
  case TOKENIZER_AREAD:
    accept(tree, TOKENIZER_AREAD);
    accept(tree, TOKENIZER_LEFTPAREN);
    // single argument: channel as hex value
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    r = analogRead(j);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
#endif
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;
#endif

  case TOKENIZER_LEFTPAREN:
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;

#if defined(VARIABLE_TYPE_ARRAY)
  case TOKENIZER_ARRAYVARIABLE:
    varnum = tokenizer_variable_num(data);
    accept(tree, TOKENIZER_ARRAYVARIABLE);
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    r = ubasic_get_arrayvariable(data, varnum, (uint16_t)j);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
  case TOKENIZER_DREAD:
    accept(tree, TOKENIZER_LEFTPAREN);
    r = relation(data);
    r = digitalRead(r);
    accept(tree, TOKENIZER_RIGHTPAREN);
    break;
#endif /* UBASIC_SCRIPT_HAVE_GPIO_CHANNELS */

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
  case TOKENIZER_RECALL:
    r = recall_statement(data);
    break;
#endif

  default:
    r = varfactor(data);
    break;
  }

  return r;
}

/*---------------------------------------------------------------------------*/

static VARIABLE_TYPE term(struct ubasic_data *data)
{
  VARIABLE_TYPE f1, f2;
  struct tokenizer_data *tree = &data->tree;

#if defined(VARIABLE_TYPE_STRING)
  if (tokenizer_stringlookahead(tree))
  {
    f1 = slogexpr(data);
  }
  else
#endif
  {
    f1 = factor(data);
    VARIABLE_TYPE op = tokenizer_token(tree);
    while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD)
    {
      tokenizer_next(tree);
      f2 = factor(data);
      switch (op)
      {

      case TOKENIZER_ASTR:
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        f1 = fixedpt_xmul(f1, f2);
#else
        f1 = f1 * f2;
#endif
        break;

      case TOKENIZER_SLASH:
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        f1 = fixedpt_xdiv(f1, f2);
#else
        f1 = f1 / f2;
#endif
        break;

      case TOKENIZER_MOD:
        f1 = f1 % f2;
        break;
      }
      op = tokenizer_token(tree);
    }
  }
  return f1;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE relation(struct ubasic_data *data)
{
  VARIABLE_TYPE r1, r2;

  r1 = (VARIABLE_TYPE)term(data);

  VARIABLE_TYPE op = tokenizer_token(&data->tree);

  while (op == TOKENIZER_LT || op == TOKENIZER_LE ||
         op == TOKENIZER_GT || op == TOKENIZER_GE ||
         op == TOKENIZER_EQ || op == TOKENIZER_NE ||
         op == TOKENIZER_LAND || op == TOKENIZER_LOR ||
         op == TOKENIZER_PLUS || op == TOKENIZER_MINUS ||
         op == TOKENIZER_AND || op == TOKENIZER_OR)
  {
    tokenizer_next(&data->tree);
    r2 = (VARIABLE_TYPE)term(data);

    switch (op)
    {
    case TOKENIZER_LE:
      r1 = (r1 <= r2);
      break;

    case TOKENIZER_LT:
      r1 = (r1 < r2);
      break;

    case TOKENIZER_GT:
      r1 = (r1 > r2);
      break;

    case TOKENIZER_GE:
      r1 = (r1 >= r2);
      break;

    case TOKENIZER_EQ:
      r1 = (r1 == r2);
      break;

    case TOKENIZER_NE:
      r1 = (r1 != r2);
      break;

    case TOKENIZER_LAND:
      r1 = (r1 && r2);
      break;

    case TOKENIZER_LOR:
      r1 = (r1 || r2);
      break;

    case TOKENIZER_PLUS:
      r1 = r1 + r2;
      break;

    case TOKENIZER_MINUS:
      r1 = r1 - r2;
      break;

    case TOKENIZER_AND:
      r1 = ((int32_t)r1) & ((int32_t)r2);
      break;

    case TOKENIZER_OR:
      r1 = ((int32_t)r1) | ((int32_t)r2);
      break;
    }
    op = tokenizer_token(&data->tree);
  }

  return r1;
}

// TODO: error handling?
uint8_t jump_label(struct ubasic_data *data, char *label)
{
  char currLabel[MAX_LABEL_LEN] = {'\0'};
  struct tokenizer_data *tree = &data->tree;

  tokenizer_init(tree, data->program_ptr);

  while (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)
  {
    tokenizer_next(tree);

    if (tokenizer_token(tree) == TOKENIZER_COLON)
    {
      tokenizer_next(tree);
      if (tokenizer_token(tree) == TOKENIZER_LABEL)
      {
        tokenizer_label(tree, currLabel, sizeof(currLabel));
        if (strcmp(label, currLabel) == 0)
        {
          accept(tree, TOKENIZER_LABEL);
          return 1;
        }
      }
    }
  }

  if (tokenizer_token(tree) == TOKENIZER_ENDOFINPUT)
    return 0;

  return 1;
}

static void gosub_statement(struct ubasic_data *data)
{
  char tmpstring[MAX_STRINGLEN];
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_GOSUB);
  if (tokenizer_token(tree) == TOKENIZER_LABEL)
  {
    // copy label
    tokenizer_label(tree, tmpstring, MAX_STRINGLEN);
    tokenizer_next(tree);

    // check for the end of line
    while (tokenizer_token(tree) == TOKENIZER_EOL)
      tokenizer_next(tree);
    //     accept_cr(tree);

    if (data->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH)
    {
      /*    tokenizer_line_number_inc();*/
      // gosub_stack[gosub_stack_ptr] = tokenizer_line_number();
      data->gosub_stack[data->gosub_stack_ptr] = tokenizer_save_offset(tree);
      data->gosub_stack_ptr++;
      jump_label(data, tmpstring);
      return;
    }
  }

  tokenizer_error_print(tree, TOKENIZER_GOSUB);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}

static void return_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_RETURN);
  if (data->gosub_stack_ptr > 0)
  {
    data->gosub_stack_ptr--;
    // jump_line(gosub_stack[gosub_stack_ptr]);
    tokenizer_jump_offset(tree, data->gosub_stack[data->gosub_stack_ptr]);
    return;
  }
  tokenizer_error_print(tree, TOKENIZER_RETURN);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}

static void goto_statement(struct ubasic_data *data)
{
  char tmpstring[MAX_STRINGLEN];
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_GOTO);

  if (tokenizer_token(tree) == TOKENIZER_LABEL)
  {
    tokenizer_label(tree, tmpstring, sizeof(MAX_STRINGLEN));
    tokenizer_next(tree);
    jump_label(data, tmpstring);
    return;
  }

  tokenizer_error_print(tree, TOKENIZER_GOTO);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/
#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
static void pwm_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE j, r;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_PWM);

  accept(tree, TOKENIZER_LEFTPAREN);

  // first argument: channel
  j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  j = fixedpt_toint(j);
#endif
  if (j < 1 || j > 4)
    return;

  accept(tree, TOKENIZER_COMMA);

  // second argument: value
  r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  r = fixedpt_toint(r);
#endif
  accept(tree, TOKENIZER_RIGHTPAREN);

  if (j >= 1 && j <= UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
  {
    analogWrite(j, r);
  }

  accept_cr(tree);
  return;
}

static void pwmconf_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE j, r;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_PWMCONF);
  accept(tree, TOKENIZER_LEFTPAREN);
  // first argument: prescaler 0...
  j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  j = fixedpt_toint(j);
#endif
  if (j < 0)
    j = 0;

  accept(tree, TOKENIZER_COMMA);
  r = relation(data);
  // second argument: period
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  r = fixedpt_toint(r);
#endif
  analogWriteConfig(j, r);
  r = 0;
  accept(tree, TOKENIZER_RIGHTPAREN);
  accept_cr(tree);
}

#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
static void areadconf_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE j, r;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_AREADCONF);
  accept(tree, TOKENIZER_LEFTPAREN);
  // first argument: sampletime 0...7 on STM32
  j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  j = fixedpt_toint(j);
#endif
  if (j < 0)
    j = 0;
  if (j > 7)
    j = 7;
  accept(tree, TOKENIZER_COMMA);
  r = relation(data);
  // second argument: number of analog sample to average from
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  r = fixedpt_toint(r);
#endif
  analogReadConfig(j, r);
  accept(tree, TOKENIZER_RIGHTPAREN);
  accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
static void pinmode_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE i, j, r;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_PINMODE);
  accept(tree, TOKENIZER_LEFTPAREN);

  // channel - should be entered as 0x..
  i = relation(data);
  if (i < 0xa0 || i > 0xff)
    return;

  accept(tree, TOKENIZER_COMMA);

  // mode
  j = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  j = fixedpt_toint(j);
#endif

  if (j < -2)
    j = -1;
  if (j > 2)
    j = 0;

  accept(tree, TOKENIZER_COMMA);

  // speed
  r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  r = fixedpt_toint(r);
#endif
  if (r < 0 || r > 2)
    r = 0;

  accept(tree, TOKENIZER_RIGHTPAREN);

  pinMode((uint8_t)i, (int8_t)j, (int8_t)r);

  accept_cr(tree);

  return;
}

static void dwrite_statemet(struct ubasic_data *data)
{
  VARIABLE_TYPE j, r;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_DWRITE);
  accept(tree, TOKENIZER_LEFTPAREN);
  j = relation(data);
  accept(tree, TOKENIZER_COMMA);
  r = relation(data);
  if (r)
    r = 0x01;
  accept(tree, TOKENIZER_RIGHTPAREN);
  r = digitalWrite(j, r);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  r = fixedpt_fromint(r);
#endif
  accept_cr(tree);

  return;
}

#endif /* UBASIC_SCRIPT_HAVE_ANALOG_READ */

static void print_statement(struct ubasic_data *data, uint8_t println)
{
  uint8_t print_how = 0; /*0-xp, 1-hex, 2-oct, 3-dec, 4-bin*/
  char tmpstring[MAX_STRINGLEN];
  struct tokenizer_data *tree = &data->tree;

  // string additions
  if (println)
    accept(tree, TOKENIZER_PRINTLN);
  else
    accept(tree, TOKENIZER_PRINT);
  do
  {
    if (tokenizer_token(tree) == TOKENIZER_PRINT_HEX)
    {
      tokenizer_next(tree);
      print_how = 1;
    }
    else if (tokenizer_token(tree) == TOKENIZER_PRINT_DEC)
    {
      tokenizer_next(tree);
      print_how = 2;
    }
#if defined(VARIABLE_TYPE_STRING)
    if (tokenizer_token(tree) == TOKENIZER_STRING)
    {
      tokenizer_string(tree, tmpstring, MAX_STRINGLEN);
      tokenizer_next(tree);
    }
    else
#endif
        if (tokenizer_token(tree) == TOKENIZER_COMMA)
    {
      sprintf(tmpstring, " ");
      tokenizer_next(tree);
    }
    else
    {
#if defined(VARIABLE_TYPE_STRING)
      if (tokenizer_stringlookahead(tree))
      {
        sprintf(tmpstring, "%s", strptr(data, sexpr(data)));
      }
      else
#endif
      {
        if (print_how == 1)
        {
          sprintf(tmpstring, "%lx", (uint32_t)relation(data));
        }
        else if (print_how == 2)
        {
          sprintf(tmpstring, "%ld", relation(data));
        }
        else
        {
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
          fixedpt_str(relation(data), tmpstring, FIXEDPT_FBITS / 3);
#else
          sprintf(tmpstring, "%ld", relation(data));
#endif
        }
      }
      // end of string additions
    }
    print_serial(tmpstring);
  } while (tokenizer_token(tree) != TOKENIZER_EOL &&
           tokenizer_token(tree) != TOKENIZER_ENDOFINPUT);

  // printf("\n");
  if (println)
    print_serial("\n");

  accept_cr(tree);
}
/*---------------------------------------------------------------------------*/
static void endif_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;
  if (data->if_stack_ptr > 0)
  {
    accept(tree, TOKENIZER_ENDIF);
    accept(tree, TOKENIZER_EOL);
    data->if_stack_ptr--;
    return;
  }
  tokenizer_error_print(tree, TOKENIZER_IF);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
  return;
}

static void if_statement(struct ubasic_data *data)
{
  uint8_t else_cntr, endif_cntr, f_nt, f_sl;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_IF);

  VARIABLE_TYPE r = relation(data);

  if (accept(tree, TOKENIZER_THEN))
  {
    tokenizer_error_print(tree, TOKENIZER_IF);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
    return;
  }

  if (tokenizer_token(tree) == TOKENIZER_EOL)
  {
    // Multi-line IF-Statement
    // CR after then -> multiline IF-Statement
    if (data->if_stack_ptr < MAX_IF_STACK_DEPTH)
    {
      data->if_stack[data->if_stack_ptr] = r;
      data->if_stack_ptr++;
    }
    else
    {
      tokenizer_error_print(tree, TOKENIZER_IF);
      data->status.bit.isRunning = 0;
      data->status.bit.Error = 1;
      return;
    }
    accept(tree, TOKENIZER_EOL);
    if (r)
      return;
    else
    {
      else_cntr = endif_cntr = 0; // number of else/endif possible in current nesting
      f_nt = f_sl = 0;            // f_nt flag for additional next token, f_fs flag single line

      while ((
                 (tokenizer_token(tree) != TOKENIZER_ELSE && tokenizer_token(tree) != TOKENIZER_ENDIF) ||
                 else_cntr || endif_cntr) &&
             tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)
      {
        f_nt = 0;

        // nested if
        if (tokenizer_token(tree) == TOKENIZER_IF)
        {
          else_cntr += 1;
          endif_cntr += 1;
          f_sl = 0;
        }

        if (tokenizer_token(tree) == TOKENIZER_THEN)
        {
          f_nt = 1;
          tokenizer_next(tree);
          if (tokenizer_token(tree) != TOKENIZER_EOL)
            f_sl = 1;
        }

        if (tokenizer_token(tree) == TOKENIZER_ELSE)
        {
          else_cntr--;
          if (else_cntr < 0)
          {
            tokenizer_error_print(tree, TOKENIZER_IF);
            data->status.bit.isRunning = 0;
            data->status.bit.Error = 1;
            return;
          }
        }

        if (!f_sl && (tokenizer_token(tree) == TOKENIZER_ENDIF))
        {
          endif_cntr--;
          if (endif_cntr != else_cntr)
            else_cntr--;
        }
        else
        {
          if (f_sl && (tokenizer_token(tree) == TOKENIZER_EOL))
          {
            f_sl = 0;
            endif_cntr--;
            if (endif_cntr != else_cntr)
              else_cntr--;
          }
          else
          {
            if (tokenizer_token(tree) == TOKENIZER_ENDIF)
            {
              tokenizer_error_print(tree, TOKENIZER_IF);
              data->status.bit.isRunning = 0;
              data->status.bit.Error = 1;
              return;
            }
          }
        }
        if (!f_nt)
          tokenizer_next(tree);
      }

      if (tokenizer_token(tree) == TOKENIZER_ELSE)
        return;
    }
    endif_statement(data);
  }
  else
  {
    // Single-line IF-Statement
    if (r)
    {
      statement(data);
    }
    else
    {
      do
      {
        tokenizer_next(tree);
      } while (tokenizer_token(tree) != TOKENIZER_ELSE &&
               tokenizer_token(tree) != TOKENIZER_EOL &&
               tokenizer_token(tree) != TOKENIZER_ENDOFINPUT);
      if (tokenizer_token(tree) == TOKENIZER_ELSE)
      {
        accept(tree, TOKENIZER_ELSE);
        tokenizer_next(tree);
        statement(data);
      }
      else
        accept_cr(tree);
    }
  }
}

static void else_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE r = 0;
  uint8_t endif_cntr, f_nt;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_ELSE);

  if (data->if_stack_ptr > 0)
  {
    r = data->if_stack[data->if_stack_ptr - 1];
  }
  else
  {
    tokenizer_error_print(tree, TOKENIZER_ELSE);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
    return;
  }
  if (tokenizer_token(tree) == TOKENIZER_EOL)
  {
    accept(tree, TOKENIZER_EOL);
    if (!r)
      return;
    else
    {
      endif_cntr = 0;
      while (((tokenizer_token(tree) != TOKENIZER_ENDIF) || endif_cntr) &&
             tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)
      {
        f_nt = 0;
        if (tokenizer_token(tree) == TOKENIZER_IF)
          endif_cntr += 1;
        if (tokenizer_token(tree) == TOKENIZER_THEN)
        {
          tokenizer_next(tree);
          // then followed by CR -> multi line
          if (tokenizer_token(tree) == TOKENIZER_EOL)
          {
            f_nt = 1;
          }
          else
          { // single line
            endif_cntr--;
            while (tokenizer_token(tree) != TOKENIZER_ENDIF &&
                   tokenizer_token(tree) != TOKENIZER_EOL &&
                   tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)
            {
              tokenizer_next(tree);
            }
            if (tokenizer_token(tree) == TOKENIZER_ENDIF)
            {
              tokenizer_error_print(tree, TOKENIZER_ELSE);
              data->status.bit.isRunning = 0;
              data->status.bit.Error = 1;
              return;
            }
          }
        }
        if (tokenizer_token(tree) == TOKENIZER_ENDIF)
          endif_cntr--;
        if (!f_nt)
          tokenizer_next(tree);
      }
    }
    endif_statement(data);
    return;
  }
  tokenizer_error_print(tree, TOKENIZER_ELSE);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
  return;
}

/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_data *data)
{
  uint8_t varnum;
  struct tokenizer_data *tree = &data->tree;

  if (tokenizer_token(tree) == TOKENIZER_VARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_VARIABLE);
    if (!accept(tree, TOKENIZER_EQ))
      ubasic_set_variable(data, varnum, relation(data));
  }
#if defined(VARIABLE_TYPE_STRING)
  // string additions here
  else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_STRINGVARIABLE);
    if (!accept(tree, TOKENIZER_EQ))
    {
      // print_serial("let_s:");
      // int16_t d=sexpr();
      // print_serial(strptr(d));
      // print_serial("\n");
      // ubasic_set_stringvariable(varnum,d);
      ubasic_set_stringvariable(data, varnum, sexpr(data));
    }
  }
  // end of string additions
#endif
#if defined(VARIABLE_TYPE_ARRAY)
  else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_ARRAYVARIABLE);

    accept(tree, TOKENIZER_LEFTPAREN);
    VARIABLE_TYPE idx = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    idx = fixedpt_toint(idx);
#endif
    accept(tree, TOKENIZER_RIGHTPAREN);
    if (!accept(tree, TOKENIZER_EQ))
      ubasic_set_arrayvariable(data, varnum, (uint16_t)idx, relation(data));
  }
#endif

  accept_cr(tree);
}

#if defined(VARIABLE_TYPE_ARRAY)
static void dim_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE size = 0;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_DIM);
  uint8_t varnum = tokenizer_variable_num(tree);
  accept(tree, TOKENIZER_ARRAYVARIABLE);

  //   accept(tree, TOKENIZER_LEFTPAREN);
  size = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  size = fixedpt_toint(size);
#endif

  ubasic_dim_arrayvariable(data, varnum, size);

  //   accept(tree, TOKENIZER_RIGHTPAREN);
  accept_cr(tree);

  // end of array additions
}
#endif

/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_NEXT);
  uint8_t var = tokenizer_variable_num(tree);
  accept(tree, TOKENIZER_VARIABLE);
  if (data->for_stack_ptr > 0 && var == data->for_stack[data->for_stack_ptr - 1].for_variable)
  {
    VARIABLE_TYPE value = ubasic_get_variable(data, var) + data->for_stack[data->for_stack_ptr - 1].step;
    ubasic_set_variable(data, var, value);

    if (((data->for_stack[data->for_stack_ptr - 1].step > 0) &&
         (value <= data->for_stack[data->for_stack_ptr - 1].to)) ||
        ((data->for_stack[data->for_stack_ptr - 1].step < 0) &&
         (value >= data->for_stack[data->for_stack_ptr - 1].to)))
    {
      // jump_line(for_stack[for_stack_ptr - 1].line_after_for);
      tokenizer_jump_offset(tree, data->for_stack[data->for_stack_ptr - 1].line_after_for);
    }
    else
    {
      data->for_stack_ptr--;
      accept_cr(tree);
    }
    return;
  }

  tokenizer_error_print(tree, TOKENIZER_FOR);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/

static void for_statement(struct ubasic_data *data)
{
  uint8_t for_variable;
  VARIABLE_TYPE to;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_FOR);
  for_variable = tokenizer_variable_num(tree);
  accept(tree, TOKENIZER_VARIABLE);
  accept(tree, TOKENIZER_EQ);
  ubasic_set_variable(data, for_variable, relation(data));
  accept(tree, TOKENIZER_TO);
  to = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  VARIABLE_TYPE step = FIXEDPT_ONE;
#else
  VARIABLE_TYPE step = 1;
#endif
  if (tokenizer_token(tree) == TOKENIZER_STEP)
  {
    accept(tree, TOKENIZER_STEP);
    step = relation(data);
  }
  accept_cr(tree);

  if (data->for_stack_ptr < MAX_FOR_STACK_DEPTH)
  {
    // for_stack[for_stack_ptr].line_after_for = tokenizer_line_number();
    data->for_stack[data->for_stack_ptr].line_after_for = tokenizer_save_offset(tree);
    data->for_stack[data->for_stack_ptr].for_variable = for_variable;
    data->for_stack[data->for_stack_ptr].to = to;
    data->for_stack[data->for_stack_ptr].step = step;
    data->for_stack_ptr++;
    return;
  }

  tokenizer_error_print(tree, TOKENIZER_FOR);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/

static void end_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_END);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 0;
}

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
static void sleep_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_SLEEP);
  VARIABLE_TYPE f = relation(data);
  if (f > 0)
  {
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    data->sleeping_ms = fixedpt_toint(f * 1000);
#else
    data->sleeping_ms = (uint32_t)f;
#endif
  }
  else
  {
    data->sleeping_ms = 0;
  }

  accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_TICTOC)
static void tic_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_TIC);
  accept(tree, TOKENIZER_LEFTPAREN);
  VARIABLE_TYPE f = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
  f = fixedpt_toint(f);
#endif

  if (f == 2)
    ubasic_script_tic1_ms = 0;
  else if (f == 3)
    ubasic_script_tic2_ms = 0;
  else if (f == 4)
    ubasic_script_tic3_ms = 0;
  else if (f == 5)
    ubasic_script_tic4_ms = 0;
  else if (f == 6)
    ubasic_script_tic5_ms = 0;
  else
    ubasic_script_tic0_ms = 0;

  accept(tree, TOKENIZER_RIGHTPAREN);
  accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
static void input_statement_wait(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  data->input_how = 0;
  accept(tree, TOKENIZER_INPUT);
  if (tokenizer_token(tree) == TOKENIZER_PRINT_HEX)
  {
    tokenizer_next(tree);
    data->input_how = 1;
  }
  else if (tokenizer_token(tree) == TOKENIZER_PRINT_DEC)
  {
    tokenizer_next(tree);
    data->input_how = 2;
  }

  if (tokenizer_token(tree) == TOKENIZER_VARIABLE)
  {
    data->input_varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_VARIABLE);
    data->input_type = 0;
  }
#if defined(VARIABLE_TYPE_STRING)
  // string additions here
  else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE)
  {
    data->input_varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_STRINGVARIABLE);
    data->input_type = 1;
  }
// end of string additions
#endif
#if defined(VARIABLE_TYPE_ARRAY)
  else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE)
  {
    data->input_varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_ARRAYVARIABLE);

    accept(tree, TOKENIZER_LEFTPAREN);
    data->input_array_index = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    data->input_array_index = fixedpt_toint(data->input_array_index);
#endif
    accept(tree, TOKENIZER_RIGHTPAREN);
    data->input_type = 2;
  }
#endif

  // get next token:
  //    CR
  // or
  //    , timeout
  if (tokenizer_token(tree) == TOKENIZER_COMMA)
  {
    accept(tree, TOKENIZER_COMMA);
    VARIABLE_TYPE r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    if (r > 0)
    {
      ubasic_script_wait_for_input_ms = r;
    }
  }

  accept_cr(tree);

  data->status.bit.WaitForSerialInput = 1;
}

static void serial_input_completed(struct ubasic_data *data)
{
  char tmpstring[MAX_STRINGLEN];

  // transfer serial input buffer to 'buf' only if something
  // has been received.
  // otherwise leave the variable content unchanged.
  if (serial_input(tmpstring, MAX_STRINGLEN) > 0)
  {
    if ((data->input_type == 0)
#if defined(VARIABLE_TYPE_ARRAY)
        || (data->input_type == 2)
#endif
    )
    {
      VARIABLE_TYPE r;
      if ((data->input_how == 1) || (data->input_how == 2))
      {
        r = atoi(tmpstring);
      }
      else
      {
        // process number
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        r = str_fixedpt(tmpstring, MAX_STRINGLEN, FIXEDPT_FBITS >> 1);
#else
        r = atoi(tmpstring);
#endif
      }

      if (data->input_type == 0)
      {
        ubasic_set_variable(data, data->input_varnum, r);
      }
#if defined(VARIABLE_TYPE_ARRAY)
      else if (data->input_type == 2)
      {
        ubasic_set_arrayvariable(data, data->input_varnum, data->input_array_index, r);
      }
#endif
    }
#if defined(VARIABLE_TYPE_STRING)
    else if (data->input_type == 1)
    {
      ubasic_set_stringvariable(data, data->input_varnum, scpy(data, tmpstring));
    }
#endif
  }
  data->status.bit.WaitForSerialInput = 0;
}

#endif /* #if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL) */

/*---------------------------------------------------------------------------*/
static void while_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE r;
  int8_t while_cntr;
  uint16_t while_offset;
  struct tokenizer_data *tree = &data->tree;

  // this is where we jump to after 'endwhile'
  while_offset = tokenizer_save_offset(tree);
  accept(tree, TOKENIZER_WHILE);
  if (data->while_stack_ptr == MAX_WHILE_STACK_DEPTH)
  {
    tokenizer_error_print(tree, TOKENIZER_WHILE);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
  }
  // this makes sure that the jump to the same while is ignored
  if ((data->while_stack_ptr == 0) ||
      ((data->while_stack_ptr > 0) &&
       (data->while_stack[data->while_stack_ptr - 1].line_while != while_offset)))
  {
    data->while_stack[data->while_stack_ptr].line_while = while_offset;
    data->while_stack[data->while_stack_ptr].line_after_endwhile = -1; // we don't know it yet
    data->while_stack_ptr++;
  }

  r = relation(data);

  if (data->while_stack_ptr == 0)
  {
    tokenizer_error_print(tree, TOKENIZER_WHILE);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
    return;
  }

  if (r)
  {
    accept_cr(tree);
    return;
  }

  if (data->while_stack[data->while_stack_ptr - 1].line_after_endwhile > 0)
  {
    // we have traversed while loop once to its end already.
    // thus we know where the loop ends. we just use that to jump there.
    tokenizer_jump_offset(tree, data->while_stack[data->while_stack_ptr - 1].line_after_endwhile);
  }
  else
  {
    // first time the loop is entered the condition is not satisfied,
    // so we gobble the lines until we reach the matching endwhile
    while_cntr = 0;
    while ((tokenizer_token(tree) != TOKENIZER_ENDWHILE || while_cntr) &&
           (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT))
    {
      if (tokenizer_token(tree) == TOKENIZER_WHILE)
        while_cntr += 1;
      if (tokenizer_token(tree) == TOKENIZER_ENDWHILE)
        while_cntr -= 1;
      tokenizer_next(tree);
    }
    data->while_stack_ptr--;
    accept(tree, TOKENIZER_ENDWHILE);
    accept(tree, TOKENIZER_EOL);
  }

  return;
}
/*---------------------------------------------------------------------------*/
static void endwhile_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_ENDWHILE);
  if (data->while_stack_ptr > 0)
  {
    // jump_line(while_stack[while_stack_ptr-1]);
    if (data->while_stack[data->while_stack_ptr - 1].line_after_endwhile == -1)
    {
      data->while_stack[data->while_stack_ptr - 1].line_after_endwhile = tokenizer_save_offset(tree);
    }
    tokenizer_jump_offset(tree, data->while_stack[data->while_stack_ptr - 1].line_while);
    return;
  }
  tokenizer_error_print(tree, TOKENIZER_FOR);
  data->status.bit.isRunning = 0;
  data->status.bit.Error = 1;
}
/*---------------------------------------------------------------------------*/

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)

static VARIABLE_TYPE recall_statement(struct ubasic_data *data)
{
  VARIABLE_TYPE rval = 0;
  uint8_t *dataptr;
  uint8_t *datalen;
  struct tokenizer_data *tree = &data->tree;

  accept(tree, TOKENIZER_RECALL);
  accept(tree, TOKENIZER_LEFTPAREN);
  if (tokenizer_token(tree) == TOKENIZER_VARIABLE)
  {
    data->varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_VARIABLE);
    dataptr = (uint8_t *)&data->variables[data->varnum];
    datalen = (uint8_t *)&rval;
    EE_ReadVariable(data->varnum, 0, dataptr, datalen);
    rval >>= 2;
  }
#if defined(VARIABLE_TYPE_STRING)
  else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE)
  {
    data->varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_STRINGVARIABLE);
    char dummy_s[MAX_STRINGLEN] = {0};
    dataptr = (uint8_t *)dummy_s;
    datalen = (uint8_t *)&rval;
    EE_ReadVariable(data->varnum, 1, dataptr, datalen);
    if (rval > 0)
    {
      ubasic_set_stringvariable(data, data->varnum, scpy(data, (char *)dummy_s));
    }

    clear_stringstack(data);
  }
#endif
#if defined(VARIABLE_TYPE_ARRAY)
  else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE)
  {
    data->varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_ARRAYVARIABLE);
    VARIABLE_TYPE dummy_a[VARIABLE_TYPE_ARRAY + 1];
    dataptr = (uint8_t *)dummy_a;
    datalen = (uint8_t *)&rval;
    EE_ReadVariable(data->varnum, 2, dataptr, datalen);
    if (rval > 0)
    {
      rval >>= 2;
      ubasic_dim_arrayvariable(data, data->varnum, rval);
      for (uint8_t i = 0; i < rval; i++)
        ubasic_set_arrayvariable(data, data->varnum, i + 1, dummy_a[i]);
    }
  }
#endif
  accept(tree, TOKENIZER_RIGHTPAREN);
  return rval;
}

static void store_statement(struct ubasic_data *data)
{
  static uint8_t varnum;
  struct tokenizer_data *tree = &data->tree;
  uint8_t *dataptr;
  uint8_t datalen;


  accept(tree, TOKENIZER_STORE);
  accept(tree, TOKENIZER_LEFTPAREN);

  if (tokenizer_token(tree) == TOKENIZER_VARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_VARIABLE);
    dataptr = (uint8_t *)&data->variables[varnum];
    EE_WriteVariable(varnum, 0, 4, dataptr);
  }
#if defined(VARIABLE_TYPE_STRING)
  else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_STRINGVARIABLE);
    dataptr = (uint8_t *)strptr(data, data->stringvariables[varnum]);
    datalen = strlen((char *)dataptr);
    EE_WriteVariable(varnum, 1, datalen, dataptr);
  }
#endif
#if defined(VARIABLE_TYPE_ARRAY)
  else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE)
  {
    varnum = tokenizer_variable_num(tree);
    accept(tree, TOKENIZER_ARRAYVARIABLE);
    datalen = 4 * (data->arrays_data[data->arrayvariable[varnum]] & 0x0000ffff);
    dataptr = (uint8_t *)data->arrays_data[data->arrayvariable[varnum]];
    EE_WriteVariable(varnum, 2, datalen, dataptr);
  }
#endif
  accept(tree, TOKENIZER_RIGHTPAREN);
  accept_cr(tree);
}
/*---------------------------------------------------------------------------*/
#endif

/*---------------------------------------------------------------------------*/
static void statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;
  uint8_t println = 0;
  VARIABLE_TYPE token;

  if (data->status.bit.Error)
    return;
  token = tokenizer_token(tree);
  switch (token)
  {
  case TOKENIZER_EOL:
    accept(tree, TOKENIZER_EOL);
    break;

  case TOKENIZER_PRINTLN:
    println = 1;
  case TOKENIZER_PRINT:
    print_statement(data, println);
    break;

  case TOKENIZER_IF:
    if_statement(data);
    break;

  case TOKENIZER_ELSE:
    else_statement(data);
    break;

  case TOKENIZER_ENDIF:
    endif_statement(data);
    break;

  case TOKENIZER_GOTO:
    goto_statement(data);
    break;

  case TOKENIZER_GOSUB:
    gosub_statement(data);
    break;

  case TOKENIZER_RETURN:
    return_statement(data);
    break;

  case TOKENIZER_FOR:
    for_statement(data);
    break;

  case TOKENIZER_NEXT:
    next_statement(data);
    break;

  case TOKENIZER_WHILE:
    while_statement(data);
    break;

  case TOKENIZER_ENDWHILE:
    endwhile_statement(data);
    break;

  case TOKENIZER_END:
    end_statement(data);
    break;

  case TOKENIZER_LET:
    accept(tree, TOKENIZER_LET); /* Fall through: Nothing to do! */
  case TOKENIZER_VARIABLE:
#if defined(VARIABLE_TYPE_STRING)
  // string addition
  case TOKENIZER_STRINGVARIABLE:
    // end of string addition
#endif
#if defined(VARIABLE_TYPE_ARRAY)
  case TOKENIZER_ARRAYVARIABLE:
#endif
    let_statement(data);
    break;

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
  case TOKENIZER_INPUT:
    input_statement_wait(data);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
  case TOKENIZER_SLEEP:
    sleep_statement(data);
    break;
#endif

#if defined(VARIABLE_TYPE_ARRAY)
  case TOKENIZER_DIM:
    dim_statement(data);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_TICTOC)
  case TOKENIZER_TIC:
    tic_statement(data);
    break;
#endif

#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
  case TOKENIZER_PWM:
    pwm_statement(data);
    break;

  case TOKENIZER_PWMCONF:
    pwmconf_statement(data);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
  case TOKENIZER_AREADCONF:
    areadconf_statement(data);
    break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
  case TOKENIZER_PINMODE:
    pinmode_statement(data);
    break;

  case TOKENIZER_DWRITE:
    dwrite_statemet(data);
    break;
#endif /* UBASIC_SCRIPT_HAVE_GPIO_CHANNELS */

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
  case TOKENIZER_STORE:
    store_statement(data);
    break;

  case TOKENIZER_RECALL:
    recall_statement(data);
    break;
#endif /* UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH */

  case TOKENIZER_CLEAR:
    ubasic_clear_variables(data);
    accept_cr(tree);
    break;

  default:
    tokenizer_error_print(tree, token);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
  }
}
/*---------------------------------------------------------------------------*/
static void numbered_line_statement(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  while (1)
  {
    if (tokenizer_token(tree) == TOKENIZER_COLON)
    {
      accept(tree, TOKENIZER_COLON);
      if (accept(tree, TOKENIZER_LABEL))
        return;
      continue;
    }
    break;
  }
  statement(data);

  return;
}

/*---------------------------------------------------------------------------*/
void ubasic_run_program(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;

  if (data->status.bit.isRunning == 0)
  {
    return;
  }
  if (data->status.bit.Error == 1)
  {
    return;
  }
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
  if (data->sleeping_ms)
    return;
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
  if (data->status.bit.WaitForSerialInput)
  {
    if (serial_input_available() == 0)
    {
      if (ubasic_script_wait_for_input_ms > 0)
        return;
    }
    serial_input_completed(data);
  }
#endif
#if defined(VARIABLE_TYPE_STRING)
  // string additions
  clear_stringstack(data);
  // end of string additions
#endif
  if (tokenizer_finished(tree))
  {
    return;
  }
  numbered_line_statement(data);
}

/*---------------------------------------------------------------------------*/
uint8_t ubasic_execute_statement(struct ubasic_data *data, char *stmt)
{
  struct tokenizer_data *tree = &data->tree;
  data->status.byte = 0;

  data->program_ptr = stmt;
  data->for_stack_ptr = data->gosub_stack_ptr = 0;
  tokenizer_init(tree, stmt);
  do
  {
#if defined(VARIABLE_TYPE_STRING)
    clear_stringstack(data);
#endif

    statement(data);

    if (data->status.bit.Error)
      break;

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    while (data->status.bit.WaitForSerialInput)
    {
      if (serial_input_available() == 0)
      {
        if (ubasic_script_wait_for_input_ms > 0)
          continue;
      }
      serial_input_completed(data);
    }
#endif

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    while (data->sleeping_ms)
    {
      /* FIXME: maybe just a return until the sleep is over? */
    }
#endif

  } while (!tokenizer_finished(tree));

  return data->status.byte;
}

/*---------------------------------------------------------------------------*/
uint8_t ubasic_waiting_for_input(struct ubasic_data *data)
{
  return (data->status.bit.WaitForSerialInput);
}

uint8_t ubasic_finished(struct ubasic_data *data)
{
  struct tokenizer_data *tree = &data->tree;
  return (tokenizer_finished(tree) || data->status.bit.isRunning == 0);
}

/*---------------------------------------------------------------------------*/

void ubasic_set_variable(struct ubasic_data *data, uint8_t varnum, VARIABLE_TYPE value)
{
  if (varnum > 0 && varnum <= MAX_VARNUM)
  {
    data->variables[varnum] = value;
  }
}

/*---------------------------------------------------------------------------*/

VARIABLE_TYPE ubasic_get_variable(struct ubasic_data *data, uint8_t varnum)
{
  if (varnum > 0 && varnum <= MAX_VARNUM)
  {
    return data->variables[varnum];
  }
  return 0;
}

#if defined(VARIABLE_TYPE_STRING)
//
// string additions
//
/*---------------------------------------------------------------------------*/
void ubasic_set_stringvariable(struct ubasic_data *data, uint8_t svarnum, int16_t svalue)
{
  if (svarnum < MAX_SVARNUM)
  {
    // was it previously allocated?
    if (data->stringvariables[svarnum] > -1)
      *(data->stringstack + data->stringvariables[svarnum]) = 0;

    data->stringvariables[svarnum] = svalue;
    if (svalue > -1)
    {
      *(data->stringstack + svalue) = svarnum + 1;
    }

    // print_serial("set_stringvar:");
    // char msg[12];
    // sprintf(msg, "[%d]", stringvariables[svarnum]);
    // print_serial(msg);
    // print_serial(strptr(stringvariables[svarnum]));
    // print_serial("\n");
  }
}

/*---------------------------------------------------------------------------*/

int16_t ubasic_get_stringvariable(struct ubasic_data *data, uint8_t varnum)
{

  if (varnum < MAX_SVARNUM)
  {
    // print_serial("get_stringvar:");
    // char msg[12];
    // sprintf(msg, "[%d]", stringvariables[varnum]);
    // print_serial(msg);
    // print_serial(strptr(stringvariables[varnum]));
    // print_serial("\n");

    return data->stringvariables[varnum];
  }

  return (-1);
}
//
// end of string additions
//
#endif

#if defined(VARIABLE_TYPE_ARRAY)
//
// array additions: works only for VARIABLE_TYPE 32bit
//  array storage:
//    1st entry:   [ 31:16 , 15:0]
//                  varnum   size
//    entries 2 through size+1 are the array elements
//  could work for 16bit values as well
/*---------------------------------------------------------------------------*/
void ubasic_dim_arrayvariable(struct ubasic_data *data, uint8_t varnum, int16_t newsize)
{
  if (varnum >= MAX_VARNUM)
    return;

  int16_t oldsize;
  int16_t current_location;

_attach_at_the_end:

  current_location = data->arrayvariable[varnum];
  if (current_location == -1)
  {
    /* does the array fit in the available memory? */
    if ((data->free_arrayptr + newsize + 1) < VARIABLE_TYPE_ARRAY)
    {
      current_location = data->free_arrayptr;
      data->arrayvariable[varnum] = current_location;
      data->arrays_data[current_location] = (varnum << 16) | newsize;
      data->free_arrayptr += newsize + 1;
      return;
    }
    return; /* failed to allocate*/
  }
  else
  {
    oldsize = data->arrays_data[current_location];
  }

  /* if size of the array is the same as earlier allocated then do nothing */
  if (oldsize == newsize)
  {
    return;
  }

  /* if this is the last array in arrays_data, just modify the boundary */
  if (current_location + oldsize + 1 == data->free_arrayptr)
  {
    if ((data->free_arrayptr - current_location + newsize) < VARIABLE_TYPE_ARRAY)
    {
      data->arrays_data[current_location] = (varnum << 16) | newsize;
      data->free_arrayptr += (newsize - oldsize);
      data->arrays_data[data->free_arrayptr] = 0;
      return;
    }

    /* failed to allocate memory */
    data->arrayvariable[varnum] = -1;
    return;
  }

  /* Array has been allocated before. It is not the last array */
  /* Thus we have to go over all arrays above the current location, and shift them down */
  data->arrayvariable[varnum] = -1;
  int16_t next_location;
  uint16_t mov_size, mov_varnum;
  next_location = current_location + oldsize + 1;
  do
  {
    mov_varnum = (data->arrays_data[next_location] >> 16);
    mov_size = data->arrays_data[next_location];

    for (uint8_t i = 0; i <= mov_size; i++)
    {
      data->arrays_data[current_location + i] = data->arrays_data[next_location + i];
      data->arrays_data[next_location + i] = 0;
    }
    data->arrayvariable[mov_varnum] = current_location;
    next_location = next_location + mov_size + 1;
    current_location = current_location + mov_size + 1;
    data->arrays_data[current_location] = 0;
  } while (data->arrays_data[next_location] > 0);
  data->free_arrayptr = current_location;

  /** now the array should be added to the end of the list:
      if there is space do it! */
  goto _attach_at_the_end;
}

void ubasic_set_arrayvariable(struct ubasic_data *data, uint8_t varnum, uint16_t idx, VARIABLE_TYPE value)
{
  int16_t array = data->arrayvariable[varnum];
  if (array < 0)
    return;
  uint16_t size = (uint16_t)data->arrays_data[array];
  if ((size < idx) || (idx < 1))
    return;

  data->arrays_data[array + idx] = value;
}

VARIABLE_TYPE ubasic_get_arrayvariable(struct ubasic_data *data, uint8_t varnum, uint16_t idx)
{
  int16_t array = data->arrayvariable[varnum];
  if (array < 0)
    return -1;
  uint16_t size = (uint16_t)data->arrays_data[array];
  if ((size < idx) || (idx < 1))
    return -1;
  return (VARIABLE_TYPE)data->arrays_data[array + idx];
}
#endif
/*---------------------------------------------------------------------------*/
