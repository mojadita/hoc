/* symbol.c -- tabla de simbolos.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Dec 27 15:16:22 -05 2024
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "hoc.h"
#include "colors.h"
#include "instr.h"
#include "scope.h"
#include "code.h"

#include "symbolP.h"

#ifndef   UQ_COL1_SYMBS /* { */
#warning  UQ_COL1_SYMBS should be included in 'config.mk'
#define   UQ_COL1_SYMBS         (-20)
#endif /* UQ_COL1_SYMBS    } */

#ifndef   UQ_COL2_SYMBS /* { */
#warning  UQ_COL2_SYMBS should be included in 'config.mk'
#define   UQ_COL2_SYMBS         (-20)
#endif /* UQ_COL2_SYMBS    } */

#ifndef   UQ_COL3_SYMBS /* { */
#warning  UQ_COL3_SYMBS should be included in 'config.mk'
#define   UQ_COL3_SYMBS         (-20)
#endif /* UQ_COL3_SYMBS    } */

#ifndef   UQ_COL4_SYMBS /* { */
#warning  UQ_COL4_SYMBS should be included in 'config.mk'
#define   UQ_COL4_SYMBS         (-20)
#endif /* UQ_COL4_SYMBS    } */

#ifndef   UQ_COL5_SYMBS /* { */
#warning  UQ_COL5_SYMBS should be included in 'config.mk'
#define   UQ_COL5_SYMBS         (-20)
#endif /* UQ_COL5_SYMBS    } */

#ifndef   UQ_BRKPT_WIDTH1 /* { */
#warning  UQ_BRKPT_WIDTH1 should be included in 'config.mk'
#define   UQ_BRKPT_WIDTH1        (-17)
#endif /* UQ_BRKPT_WIDTH1    } */

#ifndef   UQ_BRKPT_WIDTH2 /* { */
#warning  UQ_BRKPT_WIDTH2 should be included in 'config.mk'
#define   UQ_BRKPT_WIDTH2        (-17)
#endif /* UQ_BRKPT_WIDTH2    } */

/* La tabla de simbolos se gestiona como una lista
 * de simbolos, encadenados a traves de un puntero
 * en la estructura Symbol (.next)
 * Los Symbol solo pueden a;adirse a la lista, y
 * no se ha previsto ninguna funcion para borrarlos
 * con lo que da igual por donde los insertamos
 * (lo hacemos insertandolos al comienzo, que nos
 * permite hacerlo con mayor facilidad, y asi,
 * los simbolos recientes son mas accesibles que
 * los antiguos) */


#define V(_nam) { .name = #_nam, .type = _nam, }
static struct type2char {
    char *name;
    int   type;
} tab_types[] = {
    V(ERROR),
    V(CHAR),
    V(SHORT),
    V(INTEGER),
    V(FLOAT),
    V(DOUBLE),
    V(VAR),
    V(LVAR),
    V(BLTIN_FUNC),
    V(BLTIN_PROC),
    V(UNDEF),
    V(CONST),
    V(PROCEDURE),
    V(FUNCTION),
    V(TYPE),
#undef V
    {NULL, 0,}
};

const char *lookup_type(int typ)
{
    for (struct type2char *p = tab_types; p->name; p++) {
        if (typ == p->type)
            return p->name;
    }
    return "UNKNOWN";
}

void list_symbols(void)
{
    int col = 0;

    for (   Symbol *sym = get_current_symbol();
            sym != NULL;
            sym = sym->next)
    {
        /*
        printf("%s-%s\n",
                sym->help
                    ? sym->help
                    : sym->name,
                lookup_type(sym->type));
        */

        /*   80 Col  para 2 columnas en cada fila  */
        char workspace[80], *s = workspace;
        size_t sz = sizeof workspace;
        int n = snprintf(s, sz, "%s-%s",
            sym->help ? sym->help : sym->name,
            lookup_type(sym->type));
        s += n; sz -= n;
        switch(sym->type) {
            char ws_2[32];
        case VAR:
            snprintf(s, sz,
                     "(%s)",
                     sym->typref->t2i->printval(
                            *sym->defn,
                            ws_2, sizeof ws_2));
            break;

        case CONST:
            snprintf(s, sz,
                     "(%s)",
                     sym->typref->t2i->printval(
                            sym->cel,
                            ws_2, sizeof ws_2));
            break;
        } /* switch */
        printf(GREEN "%-40s" ANSI_END, workspace);
        if (++col == 2) {
            col = 0;
            puts("");
        }
    }
    if (col != 0)
        puts("");
} /* list_symbols */

int vprintf_ncols(int ncols, const char *fmt, va_list args)
{
    char workpad[160];

    vsnprintf(workpad, sizeof workpad, fmt, args);
    return printf("%*s", ncols, workpad);
} /* vprintf_ncols */

int printf_ncols(int ncols, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int ret_val = vprintf_ncols(ncols, fmt, args);
    va_end(args);
    return ret_val;
}


void list_all_symbols(Symbol *from)
{
    for (   Symbol *sym = from;
            sym != NULL;
            sym = sym->next)
    {
        /*
        printf("%s-%s\n",
                sym->help
                    ? sym->help
                    : sym->name,
                lookup_type(sym->type));
        */

        /*   1 fila para cada simbolo y 5 col para informacion del simbolo  */
        //printf_ncols(UQ_COL1_SYMBS, "%s/%s:  ", sym->name, lookup_type(sym->type));
        const Symbol *type = sym->typref;

        printf_ncols(45,
                     BRIGHT "%s" RED "/" CYAN "%s:  " ANSI_END,
                     sym->name, lookup_type(sym->type));

        char workplace[64];

        switch(sym->type) {
        case LVAR:
            type->t2i->printval(
                    *getarg(sym->offset),
                    workplace,
                    sizeof workplace);
            break;
        case VAR:
            type->t2i->printval(
                    *sym->defn,
                    workplace,
                    sizeof workplace);
            break;
        case CONST:
            type->t2i->printval(
                    sym->cel,
                    workplace,
                    sizeof workplace);
            break;
        };

        switch (sym->type) {
        case LVAR:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      type->name);
            printf_ncols(UQ_COL3_SYMBS, "    sz %zu, ",     type->t2i->size);
            printf_ncols(UQ_COL4_SYMBS, "offset %d, ",      sym->offset);
            printf_ncols(UQ_COL5_SYMBS, " value %s",        workplace);
            break;
        case VAR:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      type->name);
            printf_ncols(UQ_COL3_SYMBS, "    sz %zu, ",     type->t2i->size);
            printf_ncols(UQ_COL4_SYMBS, "   pos [%04lx], ", sym->defn - prog);
            printf_ncols(UQ_COL5_SYMBS, " value %s",        workplace);
            break;
        case CONST:
            printf_ncols(UQ_COL2_SYMBS, " value %s",        workplace);
            break;
        case BLTIN_FUNC:
        case BLTIN_PROC:
            printf_ncols(UQ_COL2_SYMBS, " index %d",        sym->bltin_index);
            printf(" %s %s(",
                   sym->typref
                       ? sym->typref->name
                       : "",
                   sym->name);
            const char *sep = "";
            for (int i = 0; i < sym->argums_len; ++i) {
                const Symbol *param = sym->argums[i],
                             *type  = param->typref;
                printf("%s%s %s", sep, type->name, param->name);
                sep = ", ";
            }
            printf(")");
            break;
        case FUNCTION:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      type->name);
            printf_ncols(UQ_COL3_SYMBS, "argums %zd",       sym->argums_len);
            break;
        case PROCEDURE:
            printf_ncols(UQ_COL2_SYMBS, "argums %zd",       sym->argums_len);
            break;
        }
        puts("");
    }
} /* list_all_symbols */

void list_variables(Symbol *from)
{
    char *sep = "[";
    for (   Symbol *sym = from;
            sym != NULL;
            sym = sym->next)
    {
        /*   1 fila para cada simbolo e informacion del simbolo  */
        char workplace[100], workplace_2[32];
        if (sym && sym->typref && sym->typref->t2i->fmt) {
            snprintf(workplace, sizeof workplace,
                CYAN " %s" ANSI_END, sym->typref->t2i->fmt);
        }

        switch (sym->type) {

        case LVAR:
            sym->typref->t2i->printval(
                    *getarg(sym->offset),
                    workplace_2,
                    sizeof workplace_2);

            fputs(sep, stdout);

            printf_ncols( UQ_BRKPT_WIDTH1,
                    GREEN "%s" ANSI_END "<%+d>",
                    sym->name, sym->offset);
            printf_ncols( UQ_BRKPT_WIDTH2,
                    "%s",
                    workplace_2);
            break;

        case VAR:
            sym->typref->t2i->printval(
                    *sym->defn,
                    workplace_2,
                    sizeof workplace_2);

            fputs(sep, stdout);
            printf_ncols( UQ_BRKPT_WIDTH1,
                    GREEN "%s" ANSI_END,
                    sym->name);
            printf_ncols( UQ_BRKPT_WIDTH2,
                    "%s",
                    workplace_2);
            break;

        case CONST:

            sym->typref->t2i->printval(
                    sym->cel,
                    workplace_2,
                    sizeof workplace_2);
            break;
        }
        sep = "][";
    }
    fputs("]\n", stdout);
} /* list_variables */
