/* cellP.h -- tipo Cell. (celda de memoria)
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:08:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CELLP_H
#define CELLP_H

#include "instr.h"

#include "cell.h"
#include "symbol.h"

/*  Celda de Memoria RAM donde se instala el programa  */
union Cell_u {
    struct {
        instr_code inst:   8;
        int        param:  24;
    };
    char         chr;
    short        sht;
    int          inum;
    long         num;
    float        flt;
    double       val;
    Cell        *cel;
    Symbol      *sym;
    const char  *str;
};

extern Cell prog[];   /* memoria de programa */

Cell val2cell_c(va_list args);
Cell val2cell_d(va_list args);
Cell val2cell_f(va_list args);
Cell val2cell_i(va_list args);
Cell val2cell_l(va_list args);
Cell val2cell_s(va_list args);

Cell val2cell(Symbol *typ, ...);

#endif /* CELLP_H */
