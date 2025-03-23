/* instr.c -- indireccion de instrucciones a estructuras
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:13:29 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include "code.h"
#include "instr.h"

#define INST(_nom)                 \
const instr _nom##_instr = {       \
    .code_id = INST_##_nom,        \
    .name    = #_nom,              \
    .exec    = _nom,               \
    .print   = _nom##_prt,         \
};
const instr STOP_instr = {
    .code_id = INST_STOP,
    .name    = "STOP",
    .exec    = NULL,
    .print   = STOP_prt,
};
#include "instrucciones.h"
#undef INST

const instr * const tabla_instrucciones[] = {
     [INST_STOP] = &STOP_instr,
#define INST(_nom) [INST_##_nom] = &_nom##_instr,
#include "instrucciones.h"
#undef  INST
};

const size_t tabla_instrucciones_len
    = sizeof tabla_instrucciones
    / sizeof tabla_instrucciones[0];
