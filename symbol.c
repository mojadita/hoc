/* symbol.c -- funciones asociadas a los simbolos.
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
#include "code.h"
#include "instr.h"
#include "symbolP.h"
#include "colors.h"
#include "dynarray.h"
#include "hoc.h"
#include "hoc.tab.h"


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
} /* lookup_type */

void list_symbols(void)
{
    int col = 0;

    for (   Symbol *p = get_current_symbol();
            p != NULL;
            p = p->next)
    {
        /*
        printf("%s-%s\n",
                p->help
                    ? p->help
                    : p->name,
                lookup_type(p->type));
        */

        /*   80 Col  para 4 columnas en cada fila  */
        char workspace[80], *s = workspace;
        size_t sz = sizeof workspace;
        int n = snprintf(s, sz, "%s-%s",
            p->help ? p->help : p->name,
            lookup_type(p->type));
        s += n; sz -= n;
        if (p->type == VAR) {
            snprintf(s, sz, "(%.5lg)", p->defn->val);
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
