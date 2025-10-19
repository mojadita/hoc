/* instr.c -- indireccion de instrucciones a estructuras
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Mar 22 12:13:29 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include "config.h"
#include "code.h"
#include "instr.h"

#define NELEM(_arr) (sizeof _arr / sizeof _arr[0])

/* LCU: Mon Mar 24 12:23:36 -05 2025
 * ARRAY con las definiciones de todas las instrucciones.
 * Este array contiene una entrada por cada instruccion,
 * conteniendo
 * * el .code_id (el numero unico que identifica la
 *   instruccion y que es tambien el indice en este array)
 * * el .name (el nombre de la instruccion, para usarlo
 *   printipalmente en la instruccion list)
 * * .exec es la funcion que se ejecuta cuando se esta
 *   ejecutando el programa.
 * * .print es la funcion que se ejecuta para imprimir
 *   el listado del programa.
 * Nota: la instruccion especial STOP, que para la maquina
 * virtual, se implementa la primera y con punteros nulos
 * a las funciones.  Este es el codigo que se genera en
 * la llamada a macro INST(_nom), definida a continuacion,
 * para luego llamar al fichero "instrucciones.h" con las
 * definiciones de las instrucciones propiamente dichas */
const instr instruction_set[] = {
#define INST(_nom,_n, ...)        \
    [INST_##_nom] = {             \
        .code_id  = INST_##_nom,  \
        .n_cells  = _n,           \
        .name     = #_nom,        \
        .exec     = _nom,         \
        .print    = _nom##_prt,   \
        __VA_ARGS__               \
    },
#define SUFF(_typ, _nom, _suf)     \
        ._suf     = _nom##_##_suf,
#include "instrucciones.h"
#undef INST
#undef SUFF
}; /* instruction_set[] */

const size_t instruction_set_len
    = NELEM(instruction_set);

/* instr.c */
