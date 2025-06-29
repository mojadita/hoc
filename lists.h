/* lists.h -- estructuras para manejo de listas.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sun Jun 29 08:15:52 -05 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef _LISTS_H
#define _LISTS_H

#include <sys/types.h>

#include "code.h"
#include "symbol.h"

typedef struct list_of_params_s list_of_params;
typedef struct param_s param;

/* podemos tener dos tipos de listas:
 * 1. listas de parametros de una funcion.  En estas listas
 *    cada elemento de la lista representa dos tokens y ademas:
 *     a. el nombre del parametro (parm_name)
 *     b. el Symbol asociado al tipo del parametro.
 *     c. el offset del simbolo en la pila de la funcion llamada.
 *        Este valor especifica el desplazamiento del parametro
 *        respecto al frame pointer, y debe ser calculado una vez
 *        que todos los parametros han sido tenidos en cuenta.
 *        Este valor se calcula restando del offset calculado de
 *        la lista de parametros el tama;o del tipo del parametro.
 *     d. Posicion del codigo de inicializacion de la variable
 *        local.  Este valor representa la posicion de comienzo
 *        del codigo de inicializacion de la variable.  Solo para
 *        listas de variables.
 * 2. Offset maximo alcanzado tras procesar toda la lista.  Este
 *    sera instalado en el simbolo asociado a la funcion y se emplea
 *    tras leer la lista de parametros para sumarlo a todos los
 *    parametros de la lista a fin de que el frame pointer finalmente
 *    tenga offset 0 (sumamos este offset a todos los parametros de
 *    la funcion a fin de que todos los parametros tengan offset
 *    positivo respecto a este registro)
 * 3. Puntero a la celda de memoria de programa donde comienza el
 *    codigo de inicializacion que da el valor inicial a una variable.
 */

struct param_s {
    Symbol     *type;      /* param type */
    const char *parm_name; /* param name */
    off_t       offset;    /* offset respect frame pointer fp */
    Cell       *init;      /* pointer to initialization code
                            * or NULL if no initialization code */
}; /* param */

struct list_of_params_s {
    param  *data;
    size_t  data_len,
            data_cap;
    off_t   offset;
    Symbol *type;
}; /* list_of_params */

list_of_params *
new_list_of_params(
        Symbol         *type);

param *
add_to_list_of_params(
        list_of_params *list,
        Symbol         *type,
        const char     *param_name,
        off_t           offset,
        Cell           *init);

#endif /* _LISTS_H */
