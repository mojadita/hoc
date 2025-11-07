/* binop_eval.h -- evaluation of binary operators.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Thu Nov  6 14:20:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329
#define BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329

#include "cellP.h"

#define BINOP_EVAL(_pfx, _sfx, _ifl, _ofl, _op) /* { */ \
ConstExpr _pfx##_binop##_sfx(const Symbol *rtyp, ConstExpr lft, ConstExpr rgt);
#define BINOP_EVAL_EXP(_sfx, _ifl, _ofl, _op, _func) /* { */ \
    BINOP_EVAL(exp, _sfx, _ifl, _ofl, _op)

#include "binop_evals.h"

#undef BINOP_EVAL_EXP /*                           } */
#undef BINOP_EVAL     /*                           } */

#endif /* BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329 */
