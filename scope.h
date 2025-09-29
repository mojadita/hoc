/* scope.h -- Scope object.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Fri Jul  4 07:07:08 -05 2025
 * Copyright: (c) 2025 Edward Rivas & Luis Colorado.  All
 *       rights reserved.
 * License: BSD
 *
 * Scope is needed to handle the value of the
 * base offset used for local variables in this scope.  Also,
 * navigating through scopes should be possible to push/pop
 * scopes as we are interpreting the code.
 * Scope is linked to it's parent scope, being the NULL scope
 * the final parent (this is, no scope at all) a new scope is
 * created on defining a new function.  A scope gives access
 * to its parent scope (in case there exists) and to the root
 * scope of this scope (the farthest parent before NULL)
 * The scope has a pointer to the last variable of the parent
 * scope, that is used to detect when we switch scopes up to
 * the root.
 *
 * Global variables are created in no scope (the null scope)
 * and a new scope (with no parent) is created on entry to the
 * function/procedure definition, and a the parent and root
 * scopes are set for this scope.  On non-root scopes, the
 * parent scope is set to the parent scope (if it exists) and
 * the root scope is set to the present scope in case we are
 * the root.  A pointer to the last variable Symbol is stored
 * and a pointer to the last variable symbol is stored as a
 * sentinel to check where in the symbol list the current
 * scope ends.
 *
 * A global function to lookup a symbol will use the last
 * registered symbol so it always be found if the current
 * symbol exists in the scope chain.
 *
 * A global function to lookup a symbol in the current scope
 * only will be available.
 *
 * A global function to push a new scope will return the
 * current scope (from which we can navigate the scope list
 * upto the root scope, or to go directly to the root scope
 * ---this scope is the main scope in the function/procedure
 * being defined---)
 *
 * A global function to pop a scope will be used when the
 * scope ends.  This will free all symbols related to the
 * current scope upto the parent scope.
 */
#ifndef SCOPE_H
#define SCOPE_H

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

typedef struct scope_s scope;

#include "code.h"
#include "symbol.h"

struct scope_s {
    Symbol *sentinel;    /* este Symbol marca el
                          * final del scope. */
    int     base_offset; /* offset base del scope,
                          * aumenta a medida que se
                          * a;aden variables al
                          * mismo. */
    int     size;        /* tama;o del scope. */
}; /* struct scope_s */

/**
 * @brief obtiene el simbolo mas recientemente
 *        definido.
 * @return la referencia al simbolo mas reciente
 *         o NULL si no hay ningun simbolo definido
 *         (Esto no deberia ocurrir ya que el programa
 *         en la inicializacion define varios simbolos
 *         "predefinidos")
 */
Symbol *get_current_symbol();

/**
 * @brief obtiene el ambito actual.
 *
 * @return el ambito actualmente activo.
 */
scope  *get_current_scope(void);

/**
 * @brief obtiene el ambito raiz.
 *
 * El ambito raiz es el ambito primero creado
 * cuando se define una funcion o un
 * procedimiento.
 *
 * @return la referencia al scope mas profundo
 *         actual o NULL si no estamos en una
 *         definicion de funcion o procedimiento
 */
scope  *get_root_scope(void);

/**
 * @brief Crea un nuevo ambito para variables
 *        locales.
 * @return retorna el ambito recien creado.
 */
scope  *start_scope(void);

/**
 * @brief Calcula el offset de una variable de tipo type
 *        en el scope actual.
 * @param type es el Symbol asociado al tipo del
 *             parametro/variable local.
 */
int scope_calculate_offset(Symbol *type);

/**
 * @brief Destruye el ambito mas reciente.
 * @return el simbolo definido en ultimo lugar en el scope
 * que se ha eliminado (este simbolo ya no se encontrara
 * en la tabla de simbolos)
 */
Symbol *end_scope(void);

/**
 * @brief Busca un simbolo en la tabla de simbolos.
 * @param sym_name es la cadena representando el nombre
 *        del simbolo que se busca.  Debe ser una
 *        cadena de caracteres previamente internalizada.
 * @return El Symbol encontrado o NULL si no existe.
 */
Symbol *lookup(
        const char *sym_name);

/**
 * @brief Busca un simbolo en la tabla de simbolos.
 *
 * La busqueda se reduce al ambito actual y la funcion
 * se usa cuando queremos saber si la definicion de un
 * simbolo resultara en una redefinicion del mismo en
 * el ambito actual o se trata de una ocultacion de un
 * simbolo de un ambito mas externo.
 * @param sym_name es el nombre del simbolo a buscar.
 *        Este debe haber sido internalizado
 *        anteriormente.
 * @return el simbolo encontrado o NULL si no existe en
 *         el ambito actual.
 */
Symbol *lookup_current_scope(
        const char *sym_name);

/**
 * @brief instala un simbolo nuevo en la tabla
 *        de simbolos.
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
Symbol *install(
        const char   *sym_name,
        int           sym_type,
        const Symbol *lvar_type);

#endif /* SCOPE_H */
