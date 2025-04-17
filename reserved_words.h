/* reserved_words.h -- types and functions of reserved_words module.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Mon Feb 24 08:12:02 EET 2025
 * Copyright (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD.
 */
#ifndef RESERVED_WORDS_H
#define RESERVED_WORDS_H

typedef struct reserved_word reserved_word;

struct reserved_word { /* reserved statements */
    const char * name;
    int          tokn;
    const char * tokn_str;
};

const reserved_word *rw_lookup(char *lexeme);

#endif /* RESERVED_WORDS_H */
