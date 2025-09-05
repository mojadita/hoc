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
    V(NUMBER),
    V(VAR),
    V(LVAR),
    V(BLTIN0),
    V(BLTIN1),
    V(BLTIN2),
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

        /*   80 Col  para 4 columnas en cada fila  */
        char workspace[80], *s = workspace;
        size_t sz = sizeof workspace;
        int n = snprintf(s, sz, "%s-%s",
            sym->help ? sym->help : sym->name,
            lookup_type(sym->type));
        s += n; sz -= n;
        if (sym->type == VAR) {
            snprintf(s, sz, "(%.5lg)", sym->defn->val);
        }
        printf(GREEN "%-20s" ANSI_END, workspace);
        if (++col == 4) {
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
        printf_ncols(45,
                     BRIGHT "%s" RED "/" CYAN "%s:  " ANSI_END,
                     sym->name, lookup_type(sym->type));
        switch (sym->type) {
        case LVAR:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      sym->typref->name);
            printf_ncols(UQ_COL3_SYMBS, "    sz %zu, ",     sym->typref->size);
            printf_ncols(UQ_COL4_SYMBS, "offset %d, ",      sym->offset);
            printf_ncols(UQ_COL5_SYMBS, "value %.5lg",     *getarg(sym->offset));
            break;
        case VAR:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      sym->typref->name);
            printf_ncols(UQ_COL3_SYMBS, "    sz %zu, ",     sym->typref->size);
            printf_ncols(UQ_COL4_SYMBS, "   pos [%04lx], ", sym->defn - prog);
            printf_ncols(UQ_COL5_SYMBS, "value %.5lg",      sym->defn->val);
            break;
        case CONST:
            printf_ncols(UQ_COL2_SYMBS, " value %.5lg",     sym->val);
            break;
        case BLTIN0: case BLTIN1: case BLTIN2:
            printf_ncols(UQ_COL2_SYMBS, " descr %s",        sym->help);
            break;
        case FUNCTION:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      sym->typref->name);
            printf_ncols(UQ_COL3_SYMBS, "argums %zd",       sym->argums_len);
            break;
        case PROCEDURE:
            printf_ncols(UQ_COL2_SYMBS, "argums %zd",      sym->argums_len);
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
        /*
        printf("%s-%s\n",
                sym->help
                    ? sym->help
                    : sym->name,
                lookup_type(sym->type));
        */

        /*   1 fila para cada simbolo e informacion del simbolo  */
        switch (sym->type) {
        case LVAR:
            fputs(sep, stdout);
            printf_ncols( UQ_BRKPT_WIDTH1,
                    GREEN "%s" ANSI_END "<%+d>",
					sym->name, sym->offset);
            printf_ncols( UQ_BRKPT_WIDTH2,
                    CYAN  " %-1.7lg" ANSI_END,
					*getarg(sym->offset) );
            break;
        case VAR:
            fputs(sep, stdout);
            printf_ncols( UQ_BRKPT_WIDTH1,
                    GREEN "%s" ANSI_END "{%04lx}",
                    sym->name, sym->defn - prog);
            printf_ncols( UQ_BRKPT_WIDTH2,
                    CYAN " %-1.7lg" ANSI_END,
                    sym->defn->val );
            break;
        }
        sep = "][";
    }
    fputs("]\n", stdout);
} /* list_variables */
