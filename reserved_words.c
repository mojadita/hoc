/* reserved_words.c -- mapping between lexemes to reserved words
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Feb 24 07:56:25 EET 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD.
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
    RW(else,   ELSE),
    RW(func,   FUNC),
    RW(if,     IF),
    RW(list,   LIST),
    RW(print,  PRINT),
    RW(proc,   PROC),
    RW(return, RETURN),
    RW(symbs,  SYMBS),
    RW(while,  WHILE),
    RW(local,  LOCAL),

    { .name = NULL }
};

const reserved_word *
rw_lookup(
        char        *lex)
{
    for (   const reserved_word *rw = reserved_words;
            rw->name;
            rw++)
        if (strcmp(rw->name, lex) == 0)
            return rw;
    return NULL;
} /* rw_lookup */
