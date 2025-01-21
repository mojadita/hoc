/* symbol.c -- tabla de symbolos.
 * Date: Fri Dec 27 15:16:22 -05 2024
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hoc.h"

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
        double      val)
{
    Symbol *ret_val = malloc(sizeof *ret_val);
    assert(ret_val != NULL);

    ret_val->name   = malloc(strlen(name)+1);
    assert(ret_val->name != NULL);
    strcpy(ret_val->name, name);

    ret_val->type   = typ;
    ret_val->u.val  = val;

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
