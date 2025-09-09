/* cell.h -- tipo Cell. (celda de memoria)
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:08:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CELL_H
#define CELL_H

typedef union Cell Cell;

#include "symbol.h"
#include "instr.h"

/*  Celda de Memoria RAM donde se instala el programa  */
union Cell {
    struct {
        instr_code inst: 8;
        int        args: 8;
        unsigned   desp: 16;
    };
    Symbol      *sym;
    double       val;
    Cell        *cel;
    const char  *str;
    long         num;
};

#endif /* CELL_H */
