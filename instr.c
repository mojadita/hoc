/* instr.c -- indireccion de instrucciones a estructuras
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:13:29 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include "code.h"
#include "instr.h"

#define NELEM(_arr) (sizeof _arr / sizeof _arr[0])

const instr instruction_set[] = {
    [INST_STOP]  = {
        .code_id = INST_STOP,
        .name    = "STOP",
        .exec    = NULL,
        .print   = NULL,
    },
#define INST(_nom)                \
    [INST_##_nom] = {             \
        .code_id  = INST_##_nom,  \
        .name     = #_nom,        \
        .exec     = _nom,         \
        .print    = _nom##_prt,   \
    },
#include "instrucciones.h"
#undef INST
}; /* instruction_set[] */

const size_t instruction_set_len
    = NELEM(instruction_set);
