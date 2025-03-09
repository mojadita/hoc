/* reserved_words.c -- mapping between lexemes to reserved words
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Feb 24 07:56:25 EET 2025
 * Copyright: (c) 2025 Luis Colorado.  All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "hoc.h"
#include "hoc.tab.h"

#include "reserved_words.h"

#define RW(_nam, _tok) {   \
		.name     = #_nam, \
		.tokn     =  _tok, \
		.tokn_str = #_tok, \
	}

static const reserved_word reserved_words[] = {
	RW(if,     IF),
	RW(else,   ELSE),
	RW(while,  WHILE),
	RW(print,  PRINT),
	RW(symbs,  SYMBS),
	RW(func,   FUNC),
	RW(proc,   PROC),
	RW(return, RETURN),
	RW(read,   READ),

	{ .name = NULL }
};

const reserved_word *
rw_lookup(
		char        *lex)
{
	for (	const reserved_word *rw = reserved_words;
			rw->name;
			rw++)
		if (strcmp(rw->name, lex) == 0)
			return rw;
	return NULL;
} /* rw_lookup */
