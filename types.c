/* types.c -- funciones especificas de cada tipo, como
 * la impresion del dato de cada tipo determinado.
 * Metodo virtual.
 *
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Oct 12 11:54:03 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdio.h>

#include "types.h"
#include "cellP.h"
#include "symbol.h"

#define PRINTVAL(_suff, _fld, _fmt) \
const char *printval##_suff(        \
        Cell    value,              \
        char   *buff,               \
        size_t  buff_sz)            \
{                                   \
    const char *ret_val = buff;     \
    snprintf(                       \
            buff,                   \
            buff_sz,                \
            _fmt,                   \
            value._fld);            \
                                    \
    return ret_val;                 \
} /* printval##_suff */

PRINTVAL(_c, chr,  FMT_CHAR)
PRINTVAL(_d, val,  FMT_DOUBLE)
PRINTVAL(_f, flt,  FMT_FLOAT)
PRINTVAL(_i, inum, FMT_INT)
PRINTVAL(_l, num,  FMT_LONG)
PRINTVAL(_s, sht,  FMT_SHORT)

/* types.c */
