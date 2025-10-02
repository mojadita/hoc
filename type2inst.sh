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

add_suffix() {
    grep "INST([a-zA-Z_][_A-Za-z0-9]*${1}," \
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

#include "type2inst.h"
#include "instr.h"

static const Cell
    one_c   = { .chr = 1 },
    one_d   = { .val = 1.0 },
    one_f   = { .flt = 1.0F },
    one_i   = { .inum = 1 },
    one_l   = { .num = 1L },
    one_s   = { .sht = 1 },
    one_str = { .str = "1" };

type2inst t2i_c = {

    .one      = &one_c,
    .fmt      = "0x%02hhx",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 0,

$(add_suffix _c)

}, t2i_d = {

    .one      = &one_d,
    .fmt      = "%.12lg",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 5,

$(add_suffix _d)

}, t2i_f = {

    .one      = &one_f,
    .fmt      = "%.7g",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 4,

$(add_suffix _f)

}, t2i_i = {

    .one      = &one_i,
    .fmt      = "%i",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 2,

$(add_suffix _i)

}, t2i_l = {

    .one      = &one_l,
    .fmt      = "%li",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 3,

$(add_suffix _l)

},  t2i_s = {

    .one      = &one_s,
    .fmt      = "0x%04hx",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 1,

$(add_suffix _s)

},  t2i_str = {

    .one      = &one_str,
    .fmt      = "%s",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_POINTER,
};

/* end of data */
EOF_b549d7fa-9e0c-11f0-9aa0-0023ae68f329
