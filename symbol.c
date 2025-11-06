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


#ifndef   UQ_TRACE_SYMBS /* { */
#warning  UQ_TRACE_SYMBS should be included in 'config.mk'
#define   UQ_TRACE_SYMBS         1
#endif /* UQ_TRACE_SYMBS    } */

#if UQ_TRACE_SYMBS /*     { */
#define SYM(_fmt, ...) printf(F(_fmt), ##__VA_ARGS__)
#else /*  UQ_TRACE_SYMBS  }{ */
#define SYM(_fmt, ...)
#endif /* UQ_TRACE_SYMBS  } */

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


/* se llama al definir una funcion (o procedimiento) */
Symbol *register_subr(
        const char   *name,   /* nombre de la funcion/procedimiento */
        int           type,   /* tipo de symbolo (PROCEDURE/FUNCTION) */
        const Symbol *typref, /* simbolo del tipo del valor devuelto por la
                             * funcion, NULL para proc */
        Cell         *entry)  /* punto de entrada a la funcion */
{
    SYM("%s(\"%s\", %s, %s, [%04lx]);\n",
            __func__,
            name,
            lookup_type(type),
            typref  ? typref->name
                    : "VOID",
            entry - prog);
    Symbol *symb = install(name, type, typref);
    symb->defn   = entry;

    return symb;
}

/* se llama al terminar la definicion de una funcion
 * (o prodecimiento) */
void end_register_subr(const Symbol *subr)
{
    /* adjust progbase to point to the code starting point */
    progbase = progp;     /* next code starts here */
    SYM("%s(%s);\n", __func__, subr->name);
}

Symbol *register_global_var(
        const char   *name,
        const Symbol *typref)
{
    assert(get_current_scope() == NULL);
    if (progp >= varbase) {
        execerror("variables zone exhausted (progp >= varbase)\n");
    }
    if (lookup(name)) {
        execerror("Variable %s already defined\n", name);
    }
    Symbol *sym = install(name, VAR, typref);
    sym->defn = --varbase;
    SYM("Symbol '%s', type=%s, typref=%s, pos=[%04lx]\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->defn ? sym->defn - prog : -1);
    return sym;
} /* register_global_var */

Symbol *register_local_var(
        const char   *name,
        const Symbol *typref)
{
    scope *scop = get_current_scope();
    assert(scop != NULL);
    if (lookup_current_scope(name)) {
        execerror("Variable " GREEN "%s" ANSI_END
            " already defined in current scope\n", name);
    }
    Symbol *sym = install(name, LVAR, typref);
    scop->size += typref->t2i->size;
    sym->offset = -(scop->base_offset + scop->size);

    SYM("Symbol '%s', type=%s, typref=%s, offset=%+d\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->offset);
    return sym;
} /* register_local_var */

Symbol *register_const(
        const char   *name,
        const Symbol *typref,
        Cell          value)
{
    scope *scop = get_current_scope();
    if (lookup_current_scope(name)) {
        execerror(GREEN "%s" ANSI_END ": cannot define constant.  "
                "Symbol already defined in same context.",
                name);
    }
    Symbol *sym = install(name, CONSTANT, typref);
    sym->cel    = value;
    char workspace[256];
    SYM("Symbol '%s', type=%s, typref=%s, value=%s\n",
        sym->name,
        lookup_type(sym->type),
        typref->name,
        sym->typref->t2i->printval(
                sym->cel,
                workspace,
                sizeof workspace));
    return sym;
} /* register_const */


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
    V(CONSTANT),
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

const char *print_prototype(const Symbol *f, char *buff, size_t buff_sz)
{
#define UPDATE()  do {                       \
            if (n >= buff_sz) n = buff_sz-1; \
            buff    += n;                    \
            buff_sz -= n;                    \
        } while (0)

    const char *ret_val = buff;

    ssize_t n;
    const Symbol *f_typref = f->typref;
    switch (f->type) {
    case BLTIN_FUNC:
    case BLTIN_PROC:
        n = snprintf(buff, buff_sz, "bltin_index <%d>:  ",
            f->bltin_index);
        UPDATE();
        break;
    }
    if (f_typref != NULL) {
        n = snprintf(buff, buff_sz, "typref %s ",
            f_typref->name);
        UPDATE();
    }
    n = snprintf(buff, buff_sz, "%s(", f->name);
    UPDATE();
    const char *sep = "";
    for (int i = 0; i < f->argums_len; ++i) {
        const Symbol *arg = f->argums[i];
        const Symbol *arg_typref = arg->typref;
        n = snprintf(buff, buff_sz, "%s%s %s",
            sep, arg_typref->name, arg->name);
        UPDATE();
        sep = ", ";
    }
    n = snprintf(buff, buff_sz, ")");
    UPDATE();
    return ret_val;
} /* print_prototype */

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

        case CONSTANT:
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
        case CONSTANT:
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
        case CONSTANT:
            printf_ncols(UQ_COL2_SYMBS, "typref %s, ",      type->name);
            printf_ncols(UQ_COL5_SYMBS, " value %s",        workplace);
            break;
        case BLTIN_FUNC:
        case BLTIN_PROC:
        case FUNCTION:
        case PROCEDURE:
            printf("%s", print_prototype(sym, workplace, sizeof workplace));
            break;
        case TYPE:
            printf_ncols(UQ_COL3_SYMBS, "    sz %zu, ", sym->t2i->size);
            printf_ncols(UQ_COL4_SYMBS, " align %zu", sym->t2i->align);
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

        case CONSTANT:

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
