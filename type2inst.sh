# type2inst.sh -- busca instrucciones de tipo _x en el fichero
#                 instrucciones.h e imprime a la salida los inicializadores
#                 de las tablas de conversion de tipo -> instruccion.
# Author: Edward Rivas <rivastkw@gmail.com>
# Date: Tue Sep 23 13:26:19 -05 2025
# Copyright: (c) 2025 Edward Rivas.  All rights reserved.
# License: BSD
#

TARGET=type2inst.c-prueba
DATE="$(LANG=C date)"
YEAR="$(LANG=C date +%Y)"

cmd() {
	grep "INST([a-zA-Z_][_A-Za-z0-9]*${1}," \
         instrucciones.h \
    | sed -e "s/INST(\([a-zA-Z_][_A-Za-z0-9]*\)${1},.*/    .\1        = instruction_set + INST_\1${1},/"
}

cat <<b549d7fa-9e0c-11f0-9aa0-0023ae68f329 #> "${TARGET}"
/* ${TARGET} -- tablas de punteros a instrucciones para
 *                cada tipo.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: ${DATE}
 * Copyright: (c) ${YEAR} Edward Rivas.  All rights reserved.
 * License: BSD
 * NOTE: This file generated automatically, don't edit.
 */

#include "type2inst.h"

type2inst t2i_c = {

$(cmd _c)

}, t2i_d = {

$(cmd _d)

}, t2i_f = {

$(cmd _d)

}, t2i_i = {

$(cmd _i)

}, t2i_l = {

$(cmd _l)

},  t2i_s = {

$(cmd _s)

}; /* t2i_s */
b549d7fa-9e0c-11f0-9aa0-0023ae68f329
