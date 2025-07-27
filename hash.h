/* hash.h -- tabla hash
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Jun  1 08:23:52 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#ifdef    __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>

/*  Declaracion para dar acceso al codigo
 *  que USA esta libreria  */
struct hash_map;

struct pair {
    const char *key;
    void       *val;
};

/* --> 0 significa igual */
typedef int  (*equal_f)(const char *a, const char *b);
typedef int  (*hash_f)(const char *a);
typedef void (*apply_f)(struct hash_map *h, struct pair *p, void *);

struct hash_map *new_hash_map(
                     size_t           buckets,
                     hash_f           hsh,
                     equal_f          equ);
void del_hash_map(   struct hash_map *map);

size_t hash_map_size(struct hash_map *map);

void *
hash_map_get(            struct hash_map *map,
                     const char      *key);

struct pair *
hash_map_get_pair(       struct hash_map *map,
                     const char      *key);

struct pair *
hash_map_put(        struct hash_map *map,
                     const char      *key,
                     void            *val);

void hash_map_apply( struct hash_map *h,
                     apply_f          to_do,
                     void            *cp);

#ifdef    __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* HASH_TABLE_H */
