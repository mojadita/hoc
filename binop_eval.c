/* binop_eval.c -- evaluation of binary operators.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Thu Nov  6 14:20:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <math.h>

#include "math.h"
#include "config.h"
#include "types.h"
#include "symbolP.h"
#include "cellP.h"
#include "hoc.h"

#ifndef   UQ_TRACE_CONST_EXPR   /* { */
#warning  UQ_TRACE_CONST_EXPR should be defined in config.mk
#define   UQ_TRACE_CONST_EXPR  (0)
#endif /* UQ_TRACE_CONST_EXPR      } */


#if UQ_TRACE_CONST_EXPR /*     { */
#define TRACE(_op, _sfx) do {                             \
        char workspace[100];                              \
        printf(F(" left=%s"),                             \
               printval##_sfx(lft.cel,                    \
                       workspace, sizeof workspace));     \
        printf(" <%s> right=%s",                          \
               #_op,                                      \
               printval##_sfx(rgt.cel,                    \
                       workspace, sizeof workspace));     \
        printf(" -> %s (%s)\n",                           \
               rtype->t2i->printval(ret_val.cel,          \
                       workspace, sizeof workspace),      \
               rtype->name);                              \
    } while (0) /* TRACE() */
#else  /* UQ_TRACE_CONST_EXPR  }{ */
#define TRACE(_op, _sfx)
#endif /* UQ_TRACE_CONST_EXPR  } */


#define BINOP_EVAL(_pfx, _sfx, _ofl, _ifl, _op) /* { */   \
ConstExpr _pfx##_binop##_sfx(                             \
        const Symbol    *rtype,                           \
        ConstExpr        lft,                             \
        ConstExpr        rgt)                             \
{                                                         \
    ConstExpr ret_val = {                                 \
        .typ = rtype,                                     \
        .cel = { ._ofl =  lft.cel._ifl _op rgt.cel._ifl },\
    };                                                    \
    TRACE(_op, _sfx);                                     \
    return ret_val;                                       \
} /* _pfx##_binop##_sfx                            } */

/* special case of exponential operator */
#define BINOP_EVAL_EXP(_sfx, _ofl, _ifl, _op, _func) /* { */  \
ConstExpr exp_binop##_sfx(                                    \
        const Symbol    *rtype,                               \
        ConstExpr        lft,                                 \
        ConstExpr        rgt)                                 \
{                                                             \
    ConstExpr ret_val = {                                     \
        .typ = rtype,                                         \
        .cel = { ._ofl = _func(lft.cel._ifl, rgt.cel._ifl) }, \
    };                                                        \
    TRACE(_op, _sfx);                                         \
    return ret_val;                                           \
} /* exp_binop##_sfx                            } */

#include "binop_evals.h"

#undef BINOP_EVAL
#undef BINOP_EVAL_EXP
#undef TRACE
