/* binop_eval.h -- evaluation of binary operators.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Thu Nov  6 14:20:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329
#define BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329

#include "cellP.h"

#define BINOP_EVAL(_pfx, _sfx, _rfl, _ofl, _op) /* { */ \
Cell _pfx##_binop_##_sfx(Symbol *rtyp, Cell lft, Cell rgt);

#include "binop_evals.h"

#undef BINOP_EVAL /*                         } */

#endif /* BINOP_EVAL_H_c633e2da_bb45_11f0_b673_0023ae68f329 */
