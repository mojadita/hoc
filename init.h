/* init.h -- inicializacion de variables globales.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Sep 15 13:20:12 -05 2025
 * Copyright: (c) 2025 Luis Colorado & Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef INIT_H
#define INIT_H

#include "symbol.h"
extern Symbol
       *Char,
       *Double,
       *Float,
       *Integer,
       *Long,
       *Short,
       *String,
       *Prev;

void init(void);  /* install constants and built-ins in table */

#endif /* INIT_H */
