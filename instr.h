/* instr.h -- definiciones de tipos y constantes para
 * el modulo instr.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:49:24 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef INSTR_H
#define INSTR_H


typedef struct instr      instr;
typedef enum   instr_code instr_code;

#include "hoc.h"

enum instr_code {
    INST_STOP,
#define INST(_nom) INST_##_nom,
#include "instrucciones.h"
#undef  INST
}; /* enum instr */

struct instr {
    instr_code    code_id;
    const char   *name;
    void        (*exec)(const instr *);
    void        (*print)(const instr *, const Cell *);
};

#define INST(_nom) extern const instr _nom##_instr;
extern const instr STOP_instr;
#include "instrucciones.h"
#undef INST

extern const instr * const tabla_instrucciones[];
extern const size_t tabla_instrucciones_len;

#endif /* INSTR_H */
