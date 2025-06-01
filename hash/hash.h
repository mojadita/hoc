/* hash.h -- tabla hash
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Jun  1 08:23:52 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <sys/types.h>

typedef int (*hash_f)(const char *str);

/* --> 0 significa igual */
typedef int (*equal_f)(const char *a, const char *b);

struct pair {
    const char *key;
    void       *val;
};

/*  Declaracion para dar acceso al codigo
 *  que USA esta libreria  */
struct hash_map;

struct hash_map *new_hash_map(
                     size_t           buckets,
                     hash_f           hsh,
                     equal_f          equ);
void del_hash_map(   struct hash_map *map);

size_t hash_map_size(struct hash_map *map);

void *
hash_get(            struct hash_map *map,
                     const char      *key);

struct pair *
hash_get_pair(       struct hash_map *map,
                     const char      *key);

struct pair *
hash_put(            struct hash_map *map,
                     const char      *key,
                     void            *val);

#endif /* HASH_TABLE_H */
