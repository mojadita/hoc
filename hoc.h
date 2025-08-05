/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Dec 27 14:57:20 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef HOC_H
#define HOC_H

#include <setjmp.h>
#include <stdio.h>

#define F(_fmt) "%s:%d: %s: "_fmt, __FILE__, __LINE__, __func__

#define OUTPUT_FMT   "%32.8g"


#include "cell.h"
#include "symbol.h"
#include "instr.h"

typedef struct gvar_decl_list_s {
	Cell   *start;
	Symbol *typref;
} gvar_decl_list;

typedef struct gvar_init_s {
	const char *name;
	Cell *start;
} gvar_init;

#include "hoc.tab.h"

/* inicializa la tabla de simbolos con las funciones builtin y las
 * variables predefinidas. */
void init(void);  /* install constants and built-ins in table */
void execerror(const char *fmt, ...);

int yyparse(void);
int yylex(void);
FILE *yysetfilename(const char *fn);
void yysetFILE(FILE *in);

extern jmp_buf begin;
extern int lineno;
extern int col_no;
extern char *progname;

typedef double Datum;

extern Datum pop(void);

extern Cell prog[];

#endif /* HOC_H */
