/* hoc.h -- types and functions related to symbol table.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *       & Edward Rivas <rivastkw@gmail.com>
 * Date: Fri Dec 27 14:57:20 -05 2024
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
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
    Cell         *start;          /* initialization code for the
                                   * initializers sequence. */
    const Symbol *type_decl;      /* variable list type */
} var_decl_list;

typedef struct var_init_s {
    const char   *name;
    Cell         *start;          /* absolute position of var in memory. */
    const Symbol *type_expr_init; /* type of the expression that calculates
                                   * the initialization code. */
} var_init;

typedef struct expr_s {
    Cell         *cel;
    const Symbol *typ;
} Expr;

typedef struct const_expr_s {
    Cell          cel;
    const Symbol *typ;
} ConstExpr;

typedef struct OpRel_s {
    Cell         *start;
    token         tok;
} OpRel;

typedef struct ConstArglist_s {
    ConstExpr    *expr_list;
    size_t        expr_list_len,
                  expr_list_cap;
} ConstArglist;

typedef ConstExpr (*bltin_const_cb)(int bltin_id, const ConstArglist args[]);

#include "hoc.tab.h"

/* initializes the symbol table with the predefined variable prev (and the
 * builtins entries (initialized from plugin initialization code) */

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
