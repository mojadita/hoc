/* hashP.h -- definiciones privadas del modulo hash.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Jun  1 08:43:17 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef HASH_TABLE_H_PRIVATE
#define HASH_TABLE_H_PRIVATE
#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "hash.h"

#include "config.h"

#ifndef   UQ_IN_UNIT_TEST
#warning  UQ_IN_UNIT_TEST should be included in config.mk
#define   UQ_IN_UNIT_TEST 0
#endif /* UQ_IN_UNIT_TEST */

#if       UQ_IN_UNIT_TEST /* { */
#define malloc mock_malloc
#define free   mock_free
#define calloc mock_calloc
#endif /* UQ_IN_UNIT_TEST    } */

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

#ifdef    __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* HASH_TABLE_H_PRIVATE */
