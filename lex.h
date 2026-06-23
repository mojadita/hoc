/* lex.h -- definiciones y tipos del scanner.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *       y Edward Rivas <rivastkw@gmail.com>
 * Date: Sun Apr  6 11:35:09 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef LEX_H_4384dee8_acea_11f0_bfa1_0023ae68f329
#define LEX_H_4384dee8_acea_11f0_bfa1_0023ae68f329

#include "cellP.h"

typedef struct token token;

struct token {
    const char *lex;  /* lexeme */
    size_t      len;  /* length */
    int         lin,  /* start line */
                col;  /* start column in line */
    int         id;   /* token type. */
};

const token *get_last_token(
        unsigned pos);

/************************************
                       UQ_LAST_TOKENS_SZ(=8)
      +--------------+    ^
      |FFFFFFFFFFFFFF|    |
      +--------------+    |
      |FFFFFFFFFFFFFF| <-[+]--- pos(=2)
      +--------------+    |
      |FFFFFFFFFFFFFF|    |
      +--------------+    | +----+
      |FFFFFFFFFFFFFF| <--+-- lt |
      +--------------+      +----+
      |              |
      +--------------+
      |              |
      +--------------+
      |              | <-[+]--- pos(=5)
      +--------------+    |
      |              | last_tokens[]
      +--------------+    ^

 ************************************/

#endif /* LEX_H_4384dee8_acea_11f0_bfa1_0023ae68f329 */
