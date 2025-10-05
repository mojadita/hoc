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

typedef void (*typeinfo_cb)(const Symbol *type, Cell value);

struct type2inst_s {
    const instr
        *const constpush, *const add,       *const sub,
        *const mul,       *const divi,      *const mod,
        *const neg,       *const pwr,       *const eval,
        *const assign,    *const print,     *const ge,
        *const le,        *const gt,        *const lt,
        *const eq,        *const ne,        *const not,
        *const argeval,   *const argassign, *const prexpr,
        *const inceval,   *const evalinc,   *const deceval,
        *const evaldec,   *const addvar,    *const subvar,
        *const mulvar,    *const divvar,    *const modvar,
        *const pwrvar,    *const arginc,    *const incarg,
        *const decarg,    *const argdec,    *const addarg,
        *const subarg,    *const mularg,    *const divarg,
        *const modarg,    *const pwrarg;

    const Cell        one;
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

#endif /* TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329 */
