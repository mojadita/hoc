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
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 0,

$(add_suffix _c)

}, t2i_d = {

    .one      = { .val = 1.0 },
    .zero     = { .val = 0.0 },
    .fmt      = "%.12lg",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 5,

$(add_suffix _d)

}, t2i_f = {

    .one      = { .flt = 1.0F },
    .zero     = { .flt = 0.0F },
    .fmt      = "%.7g",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_FLOATING_POINT,
    .weight   = 4,

$(add_suffix _f)

}, t2i_i = {

    .one      = { .inum = 1 },
    .zero     = { .inum = 0 },
    .fmt      = "%i",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 2,

$(add_suffix _i)

}, t2i_l = {

    .one      = { .num = 1L },
    .zero     = { .num = 0L },
    .fmt      = "%li",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 3,

$(add_suffix _l)

},  t2i_s = {

    .one      = { .sht = 1 },
    .zero     = { .sht = 0 },
    .fmt      = "0x%04hx",
    .printval = NULL,
    .size     = 1,
    .align    = 1,
    .flags    = TYPE_IS_INTEGER,
    .weight   = 1,

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
