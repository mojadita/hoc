/* cellP.h -- tipo Cell. (celda de memoria)
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Aug  4 11:08:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef CELLP_H_c5ba43da_ace0_11f0_8ed7_0023ae68f329
#define CELLP_H_c5ba43da_ace0_11f0_8ed7_0023ae68f329

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
    int          itg;
    long         lng;
    float        flt;
    double       dbl;
    Cell        *cel;
    Symbol      *sym;
    const char  *str;
};

extern Cell prog[];   /* memoria de programa */

#endif /* CELLP_H_c5ba43da_ace0_11f0_8ed7_0023ae68f329 */
