/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Dec 27 14:57:20 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef HOC_H_f2663572_ace7_11f0_939a_0023ae68f329
#define HOC_H_f2663572_ace7_11f0_939a_0023ae68f329

#include <setjmp.h>
#include <stdio.h>

#include "config.h"
#include "cell.h"
#include "symbol.h"
#include "instr.h"
#include "lex.h"

#if UQ_USE_LOCUS /* {{ */
#define F(_fmt) "%s:%d: %s: "_fmt, __FILE__, __LINE__, __func__
#else /* UQ_USE_LOCUS }{*/
#define F(_fmt) _fmt
#endif /* UQ_USE_LOCUS  }}*/

#define OUTPUT_FMT   "%32.8g"

typedef struct var_decl_list_s {
    Cell         *start;          /* codigo de inicializacion de la
                                   * secuencia de inicializadores */
    const Symbol *type_decl;      /* tipo de la lista de variables */
} var_decl_list;

typedef struct var_init_s {
    const char   *name;
    Cell         *start;          /* posicion absoluta de la variable
                                   * en memoria (variables globales) */
    const Symbol *type_expr_init; /* tipo de la expression que calcula
                                   * el codigo de inicializacion */
} var_init;

typedef struct expr_s {
    Cell         *cel;
    const Symbol *typ;
} Expr;

typedef struct OpRel_s {
    Cell         *start;
    token         tok;
} OpRel;

#include "hoc.tab.h"

/* inicializa la tabla de simbolos con las funciones builtin y las
 * variables predefinidas. */

void execerror(const char *fmt, ...);

int yyparse(void);
int yylex(void);
FILE *yysetfilename(const char *fn);
void yysetFILE(FILE *in);

extern jmp_buf begin;
extern int lineno;
extern int col_no;
extern char *progname;

#endif /* HOC_H_f2663572_ace7_11f0_939a_0023ae68f329 */
