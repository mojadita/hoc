/* hashP.h -- definiciones privadas del modulo hash.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Jun  1 08:43:17 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef HASH_TABLE_H_PRIVATE
#define HASH_TABLE_H_PRIVATE

#include "hash.h"

#include "config.h"

#ifndef   UQ_MAX_ALLOWED_COLLISSIONS /* { */
#warning  UQ_MAX_ALLOWED_COLLISSIONS deberia incluirse en config.mk
#define   UQ_MAX_ALLOWED_COLLISSIONS (6)
#endif /* UQ_MAX_ALLOWED_COLLISSIONS    } */

/* declaraciones para dar acceso al codigo que IMPLEMENTA
 * esta libreria */

struct bucket {
    struct pair elem[UQ_MAX_ALLOWED_COLLISSIONS];
    size_t      elem_len;
};

struct hash_map {   /*  Tabla de Hash  */
    size_t         size;
    struct bucket *buckets;
    size_t         buckets_len;
    hash_f         hash;      /*  Puntero a Funcion  */
    equal_f        equal;     /*  Puntero a funcion  */
};

#endif /* HASH_TABLE_H_PRIVATE */
