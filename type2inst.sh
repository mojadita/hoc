# type2inst.sh -- busca instrucciones de tipo _x en el fichero
#                 instrucciones.h e imprime a la salida los inicializadores
#                 de las tablas de conversion de tipo -> instruccion.
# Author: Edward Rivas <rivastkw@gmail.com>,
#         Luis Colorado <luiscoloradourcola@gmail.com>
# Date: Tue Sep 23 13:26:19 -05 2025
# Copyright: (c) 2025 Edward Rivas & Luis Colorado.  All rights reserved.
# License: BSD
#

TARGET=type2inst.c
DATE="$(LANG=C date)"
YEAR="$(LANG=C date +%Y)"

sp='[ 	]*'
id='[a-zA-Z_][a-zA-Z0-9_]*'
comma="${sp},${sp}"
suff='_[cdfils]'

binop_extract() {
	grep "^\s*BINOP_EVAL(${sp}${id}${comma}${1}${comma}" <binop_evals.h \
	| sed -E "s/^${sp}BINOP_EVAL\(${sp}(${id})${comma}(${suff})${comma}.*$/    .\1_binop = \1_binop\2,/"
	grep "^\s*BINOP_EVAL_EXP(${1}${comma}" <binop_evals.h \
	| sed -E "s/^${sp}BINOP_EVAL_EXP\((${suff})${comma}.*$/    .exp_binop = exp_binop\1,/"
}

add_suffix() {
    grep "^INST([a-zA-Z_][_A-Za-z0-9]*${1}," \
         instrucciones.h \
    | sed -e "s/INST(\([a-zA-Z_][_A-Za-z0-9]*\)${1},.*/    .\1        = instruction_set + INST_\1${1},/"
}

cat <<EOF_b549d7fa-9e0c-11f0-9aa0-0023ae68f329
/* ${TARGET} -- tablas de punteros a instrucciones para
 *                cada tipo.
 * Author: Edward Rivas <rivastkw@gmail.com>,
 *         Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: ${DATE}
 * Copyright: (c) ${YEAR} Edward Rivas & Luis Colorado.
 *            All rights reserved.
 * License: BSD
 * NOTE: This file generated automatically, don't edit.
 */

#include <sys/types.h>
#include <stdlib.h>

#include "types.h"
#include "instr.h"
#include "cellP.h"

type2inst t2i_c = {

    .one      = { .chr = 1 },
    .zero     = { .chr = 0 },
    .fmt      = "0x%02hhx",
    .printval = printval_c,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 0,

$(binop_extract _c)

$(add_suffix _c)

}, t2i_d = {

    .one      = { .dbl = 1.0 },
    .zero     = { .dbl = 0.0 },
    .fmt      = "%.12lg",
    .printval = printval_d,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 5,

$(binop_extract _d)

$(add_suffix _d)

}, t2i_f = {

    .one      = { .flt = 1.0F },
    .zero     = { .flt = 0.0F },
    .fmt      = "%.7g",
    .printval = printval_f,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 4,

$(binop_extract _f)

$(add_suffix _f)

}, t2i_i = {

    .one      = { .itg = 1 },
    .zero     = { .itg = 0 },
    .fmt      = "%i",
    .printval = printval_i,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 2,

$(binop_extract _i)

$(add_suffix _i)

}, t2i_l = {

    .one      = { .lng = 1L },
    .zero     = { .lng = 0L },
    .fmt      = "%li",
    .printval = printval_l,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 3,

$(binop_extract _l)

$(add_suffix _l)

},  t2i_s = {

    .one      = { .sht = 1 },
    .zero     = { .sht = 0 },
    .fmt      = "0x%04hx",
    .printval = printval_s,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 1,

$(binop_extract _s)

$(add_suffix _s)

},  t2i_str = {

    .one      = { .str = "one" },
    .zero     = { .str = NULL },
    .fmt      = "%s",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_POINTER,
};

/* end of data */
EOF_b549d7fa-9e0c-11f0-9aa0-0023ae68f329
