/* cell.h -- tipo Cell. (celda de memoria)
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:08:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CELL_H
#define CELL_H

typedef union Cell_u Cell;

#include "instr.h"
#include "symbol.h"

/*  Celda de Memoria RAM donde se instala el programa  */
union Cell_u {
    struct {
        instr_code inst:   12;
        int        param:  20;
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

#endif /* CELL_H */
