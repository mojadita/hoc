/* symbol.c -- tabla de symbolos.
 * Date: Fri Dec 27 15:16:22 -05 2024
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hoc.h"
#include "config.h"
#include "colors.h"
#include "hoc.tab.h"

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

static Symbol *lista_simbolos = NULL;

/**
 * @brief instala un simbolo nuevo en la tabla.
 *
 * @param name es el nombre del nuevo simbolo
 * @param typ es el tipo del simbolo.
 * @param val es el valor a asignar al nuevo
 *            simbolo, cuando el tipo es VAR.
 * @param ptr es el puntero a la funcion que
 *            calculara el valor de la expre-
 *            sion cuando se seleccione este
 *            simbolo.
 * @return La funcion retorna un puntero al
 *         nuevo Symbol creado. */
Symbol *
install(
        const char *name,
        int         typ,
        double      val,
        const char *help)
{
    Symbol *ret_val = malloc(sizeof *ret_val);
    assert(ret_val != NULL);

    ret_val->name   = strdup(name);
    assert(ret_val->name != NULL);

    ret_val->type   = typ;
    ret_val->val  = val;
    ret_val->help   = help;

    ret_val->next   = lista_simbolos;
    lista_simbolos  = ret_val;

    return ret_val;
} /* install */

/**
 * @brief busca un simbolo en la lista.
 * @param name el nombre del simbolo que
 *        buscamos.
 * @return El puntero al Symbol encontrado,
 *         o NULL, en caso de que el Symbol
 *         no exista.
 */
Symbol *lookup(
        const char *name)
{
    for (   Symbol *p = lista_simbolos;
            p != NULL;
            p = p->next)
    {
        if (strcmp(name, p->name) == 0)
            return p;
    }

    /* p == NULL */
    return NULL;
} /* lookup */

#define V(_nam) { .name = #_nam, .type = _nam, }
static struct type2char {
    char *name;
    int type;
} tab_types[] = {
    V(ERROR),
    V(NUMBER),
    V(VAR),
    V(BLTIN0),
    V(BLTIN1),
    V(BLTIN2),
    V(UNDEF),
    V(CONST),
    V(PROCEDURE),
    V(FUNCTION),
#undef V
    {NULL, 0,}
};

static const char *lookup_type(int typ)
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

    for (   Symbol *p = lista_simbolos;
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
            snprintf(s, sz, "(%.5lg)", p->val);
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

