/* hash.c-- implementacion modulo hash.c
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Jun  1 09:51:22 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hashP.h"

struct hash_map *
new_hash_map(
        size_t  buckets,
        hash_f  hsh,
        equal_f equ)
{
    struct hash_map *ret_val = malloc(sizeof *ret_val);
    assert(ret_val != NULL);
    ret_val->buckets     = calloc(buckets,
            sizeof *ret_val->buckets);
    assert(ret_val->buckets != NULL);
    ret_val->size        = 0;
    ret_val->buckets_len = buckets;
    ret_val->hash        = hsh;
    ret_val->equal       = equ;
    return ret_val;
} /* new_hash_map */

void
del_hash_map(
        struct hash_map *map)
{
    free(map->buckets);
    free(map);
} /* del_hash_map */

static struct bucket *
get_bucket(struct hash_map *map,
        const char *key)
{
    int hsh = map->hash(key);
    int bkt = hsh % map->buckets_len;
    return map->buckets + bkt;
} /* get_bucket */

static struct pair *
hash_get_pair_internal(
        equal_f          eq,
        struct bucket   *b,
        const char      *key)
{
    struct pair *end = b->elem + b->elem_len;

    for (struct pair *p = b->elem; p < end; ++p)
        if (eq(p->key, key) == 0)
            return p;
    return NULL;
} /* hash_get_pair_internal */

struct pair *
hash_map_get_pair(
        struct hash_map *map,
        const char *key)
{
    return hash_get_pair_internal(map->equal, get_bucket(map, key), key);
}

void *
hash_map_get(
        struct hash_map *map,
        const char      *key)
{
    struct pair *p = hash_get_pair_internal(map->equal, get_bucket(map, key), key);
    return p ? p->val : NULL;
} /* hash_get */

struct pair *
hash_map_put(
        struct hash_map *map,
        const char      *key,
        void            *val)
{
    struct bucket *b = get_bucket(map, key);
    struct pair *p = hash_get_pair_internal(map->equal, b, key);
    if (p) {
        p->val = val;
        return p;
    } else {
        if (b->elem_len < UQ_MAX_ALLOWED_COLLISSIONS) {
            p = b->elem + b->elem_len++;
        } else {
            return NULL;
        }
        p->key = strdup(key);
        p->val = val;
        map->size++;
    }
    return p;
}

size_t
hash_map_size(
        struct hash_map *map)
{
    return map->size;
}

void hash_map_apply(
        struct hash_map *h,
        apply_f          to_do,
        void            *cp)
{
    struct bucket *b = h->buckets;
    for (int i = 0; i < h->buckets_len; ++i, ++b) {
        struct pair *p = b->elem;
        for (int j = 0; j < b->elem_len; ++j, ++p) {
            to_do(h, p, cp);
        }
    }
} /* hash_map_apply */
