/* types.h -- type definitions used by type related things.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Sep 29 04:48:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329
#define TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329

#include "config.h"
#include "cellP.h"
#include "instr.h"
#include "symbol.h"


#ifndef   FMT_CHAR  /* { */
#warning  FMT_CHAR should be defined in config.mk
#define   FMT_CHAR "0x%02hhx"
#endif /* FMT_CHAR     } */

#ifndef   FMT_DOUBLE  /* { */
#warning  FMT_DOUBLE should be defined in config.mk
#define   FMT_DOUBLE "%.15lg"
#endif /* FMT_DOUBLE     } */

#ifndef   FMT_FLOAT  /* { */
#warning  FMT_FLOAT should be defined in config.mk
#define   FMT_FLOAT "%.7g"
#endif /* FMT_FLOAT     } */

#ifndef   FMT_INT  /* { */
#warning  FMT_INT should be defined in config.mk
#define   FMT_INT "%i"
#endif /* FMT_INT     } */

#ifndef   FMT_LONG  /* { */
#warning  FMT_LONG should be defined in config.mk
#define   FMT_LONG "%liL"
#endif /* FMT_LONG     } */

#ifndef   FMT_SHORT  /* { */
#warning  FMT_SHORT should be defined in config.mk
#define   FMT_SHORT "0x%04hx"
#endif /* FMT_SHORT     } */

#define TYPE_IS_INTEGER         (1 << 0)
#define TYPE_IS_FLOATING_POINT  (1 << 1)
#define TYPE_IS_POINTER         (1 << 2)

typedef struct type2inst_s type2inst;

typedef const char *(*typeinfo_cb)(
        Cell          value,
        char         *buff,
        size_t        buff_sz);

struct type2inst_s {
    const instr
        *const constpush, *const add,       *const sub,
        *const mul,       *const divi,      *const mod,
        *const neg,       *const pwr,       *const eval,
        *const assign,    *const print,     *const ge,
        *const le,        *const gt,        *const lt,
        *const eq,        *const ne,        *const argeval,
        *const argassign, *const prexpr,    *const bit_not,
        *const bit_or,    *const bit_xor,   *const bit_and,
        *const bit_shl,   *const bit_shr;

    const Cell        one,
                      zero;
    const char *const fmt;
    typeinfo_cb       printval;
    const size_t      size,
                      align;
    const int         flags;
    const int         weight;
};

extern type2inst t2i_c, t2i_s, t2i_i, t2i_l, t2i_f, t2i_d, t2i_str;

extern const Symbol
       *Char,
       *Double,
       *Float,
       *Integer,
       *Long,
       *Short,
       *String,
       *Prev;

const char * printval_c(Cell value, char *buff, size_t buff_sz);
const char * printval_d(Cell value, char *buff, size_t buff_sz);
const char * printval_f(Cell value, char *buff, size_t buff_sz);
const char * printval_i(Cell value, char *buff, size_t buff_sz);
const char * printval_l(Cell value, char *buff, size_t buff_sz);
const char * printval_s(Cell value, char *buff, size_t buff_sz);

#endif /* TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329 */
