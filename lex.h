/* lex.h -- definiciones y tipos del scanner.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sun Apr  6 11:35:09 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */
#ifndef LEX_H_4384dee8_acea_11f0_bfa1_0023ae68f329
#define LEX_H_4384dee8_acea_11f0_bfa1_0023ae68f329

#include "cellP.h"

typedef struct token token;

struct token {
    const char *lex;  /* lexema */
    size_t      len;  /* longitud */
    int         lin,  /* linea de comienzo */
                col;  /* columna de comienzo */
    int         id;   /* tipo de token */
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
