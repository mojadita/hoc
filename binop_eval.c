/* binop_eval.c -- evaluation of binary operators.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Thu Nov  6 14:20:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#include "cellP.h"

#define BINOP_EVAL(_pfx, _sfx, _ofl, _ifl, _op) /* { */   \
ConstExpr _pfx##_binop_##_sfx(                            \
        Symbol    *rtype,                                 \
        ConstExpr  lft,                                   \
        ConstExpr  rgt)                                   \
{                                                         \
    ConstExpr ret_val = {                                 \
        .typ = rtype,                                     \
        .cel = { ._ofl = lft.cel._ifl _op rgt.cel._ifl }, \
    };                                                    \
    return ret_val;                                       \
} /* _pfx##_binop_##_sfx                           } */

#include "binop_evals.h"

#undef BINOP_EVAL
