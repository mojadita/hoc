/* intern.c -- implementacion de la funcion intern().
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Jun  2 12:07:03 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <string.h>

#include "hash.h"
#include "intern.h"

#include "config.h"

static struct hash_map *strings;

static int
hash(const char *s)
{
	int c, ret_val=119;
	while ((c = *s++) != 0) {
		ret_val *= 23;
		ret_val += c;
		ret_val %= 1337;
	}
	return ret_val;
} /* hash */

const char *
intern(const char *s)
{
	if (strings == NULL) {
		strings = new_hash_map(UQ_HASH_MAX_BUCKETS, hash, strcmp);
		assert(strings != NULL);
	}

	struct pair *p = hash_map_get_pair(strings, s);

	if (!p) { /* no existe */
		s = strdup(s);
		p = hash_map_put(strings, s, s);
	}

	return p->val;
} /* intern */
