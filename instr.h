/* instr.h -- definiciones de tipos y constantes para
 * el modulo instr.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:49:24 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef INSTR_H
#define INSTR_H

#define I_DATA(i) (instruction_set + (i))

typedef struct instr      instr;
typedef enum   instr_code instr_code;

#include "hoc.h"

/* LCU: Mon Mar 24 12:27:57 -05 2025
 * este tipo enumerado se introduce a partir de la macro
 * INST(_nom) y del fichero de instrucciones "instrucciones.h"
 * Cada instruccion se  define como una constante de nombre
 * INST_<_nom> en dicha enumeracion, que permite crear un
 * y asignar los datos de cada instruccion en la posicion
 * correcta, correspondiente segun el orden en que se han
 * definido las instrucciones en el fichero mencionado
 * "instrucciones.h" */
enum instr_code {
#define INST(_nom) INST_##_nom,
#include "instrucciones.h"
#undef  INST
}; /* enum instr */

struct instr {
    instr_code    code_id;
    const char   *name;
    void        (*exec)(const instr *);
    void        (*print)(const instr *, const Cell **);
};

extern const instr  instruction_set[];
extern const size_t instruction_set_len;

#endif /* INSTR_H */
