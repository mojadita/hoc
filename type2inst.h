/* type2inst -- codigo de instruccion correspondiente a la
 *              instruccion nombrada en el campo para el tipo
 *              al que se asigne la estructura.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Tue Sep 23 08:46:37 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef TYPE2INST_H
#define TYPE2INST_H

typedef struct type2inst_s type2inst;

#include "instr.h"

struct type2inst_s {
    const instr
        *constpush, *add, *sub, *mul, *divi, *mod, *neg, *pwr, *eval, *assign,
        *print, *ge, *le, *gt, *lt, *eq, *ne, *not, *argeval, *argassign, *prexpr,
		*inceval, *evalinc, *deceval, *evaldec, *addvar, *subvar, *mulvar, *divvar,
        *modvar, *pwrvar, *arginc, *incarg, *decarg, *argdec, *addarg, *subarg,
        *mularg, *divarg, *modarg, *pwrarg;
};

extern type2inst t2i_c, t2i_s, t2i_i, t2i_l, t2i_f, t2i_d;

#endif /* TYPE2INST_H */
