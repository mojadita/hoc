%{
/* lex.l -- scanner lexico para hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Apr  5 13:31:10 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "code.h"
#include "lex.h"
#include "reserved_words.h"
#include "scope.h"
#include "intern.h"

#ifndef   UQ_LEX_DEBUG
#warning  UQ_LEX_DEBUG deberia ser configurado en config.mk
#define   UQ_LEX_DEBUG 0
#endif

#ifndef   UQ_LEX_COMMENTS /* {{ */
#warning  UQ_LEX_COMMENTS deberia ser configurado en config.mk
#define   UQ_LEX_COMMENTS 0
#endif /* UQ_LEX_COMMENTS    }} */

#ifndef   UQ_LAST_TOKENS_SZ /* {{ */
#warning  UQ_LAST_TOKENS_SZ deberia ser configurado en config.mk
#define   UQ_LAST_TOKENS_SZ   (16)
#endif /* UQ_LAST_TOKENS_SZ    }} */

#ifndef   UQ_TAB_SIZE  /* {{ */
#warning  UQ_TAB_SIZE deberia ser configurado en config.mk
#define   UQ_TAB_SIZE    (8)
#endif /* UQ_TAB_SIZE     }} */

#if UQ_LEX_DEBUG  /*   {{ */

# define P_TAIL(  _fmt, ...) \
         printf(  _fmt,    ##__VA_ARGS__)
# define P(       _fmt, ...) \
         printf(F(_fmt), ##__VA_ARGS__)

#else /* UQ_LEX_DEBUG  }{ */

# define P(       _fmt, ...)
# define P_TAIL(  _fmt, ...)

#endif /* UQ_LEX_DEBUG }} */

#if       UQ_LEX_COMMENTS /* {{ */
# define C_TAIL(  _fmt, ...)  \
         printf(  _fmt, ##__VA_ARGS__)
# define C(       _fmt, ...)  \
         printf(  _fmt, ##__VA_ARGS__)
#else  /* UQ_LEX_COMMENTS    }{ */
# define C_TAIL(_fmt, ...)
# define C(     _fmt, ...)
#endif /* UQ_LEX_COMMENTS    }} */

static const token *
add_token(
        int         tok,
        int         lin,
        int         col,
        size_t      len,
        const char *lex);

int lineno = 1;
int col_no = 1;

/* declaracion adelantada */
static char *deescape(char *in);

%}

%option noyywrap
%x COMMENT

%%

" "+          {  col_no += yyleng; /* saltar blancos y tabuladores */ }
\t            {  col_no += UQ_TAB_SIZE; col_no -= col_no % UQ_TAB_SIZE; }
\\\n          {  col_no = 1; lineno++; }

0[xX][0-9a-fA-F]* {
                add_token(INTEGER, lineno, col_no, yyleng, yytext);
                col_no += yyleng;
                /* numero entero hex */
                sscanf(yytext, "%x", &yylval.num);
                P("integer %d (%d)\n", yylval.num, INTEGER);

                return INTEGER;
              }   /* Un numero integer (hex) */

0[0-7]* {
                add_token(INTEGER, lineno, col_no, yyleng, yytext);
                col_no += yyleng;
                /* numero entero octal */
                sscanf(yytext, "%o", &yylval.num);
                P("integer %d (%d)\n", yylval.num, INTEGER);

                return INTEGER;
              }   /* Un numero integer (octal) */

[1-9][0-9]* {
                add_token(INTEGER, lineno, col_no, yyleng, yytext);
                col_no += yyleng;
                /* numero entero decimal */
                sscanf(yytext, "%d", &yylval.num);
                P("integer %d (%d)\n", yylval.num, INTEGER);

                return INTEGER;
              }   /* Un numero integer (decimal) */

([0-9]+\.|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
                add_token(NUMBER, lineno, col_no, yyleng, yytext);
                col_no += yyleng;
                /* numero en punto flotante */
                sscanf(yytext, "%lf", &yylval.val);
                P("number %g (%d)\n", yylval.val, NUMBER);

                return NUMBER;
              }   /* Un numero punto flotante  */

"/*"          { /* comienzo de un comentario */
                BEGIN COMMENT;
                C(     "%s", yytext);
                col_no += yyleng;
              }
<COMMENT>.    { C_TAIL("%s", yytext);
                col_no += yyleng;
              }
<COMMENT>\n   { C_TAIL("\n");
                C("");
                lineno++;
                col_no = 1;
              }
<COMMENT>"*/" { /* FIN DE UN COMENTARIO */
                C_TAIL("%s\n", yytext);
                BEGIN INITIAL;
                col_no += yyleng;
              }

[a-zA-Z_][a-zA-Z0-9_]*  {
            /* PALABRA RESERVADA */
            int saved_col_no = col_no;
            col_no          += yyleng;
            const char *lexema = intern(yytext);
            const reserved_word *rw = rw_lookup(lexema);
            if (rw) {
                P("reserved word '%s' -> %s(%d)\n",
                    rw->name, rw->tokn_str, rw->tokn);
                add_token(rw->tokn, lineno, saved_col_no, yyleng, lexema);

                return rw->tokn;
            }

            /* BUSCAMOS EN LA TABLA DE SIMBOLOS */
            Symbol *s;
            if ((s = lookup(lexema)) != NULL) {
                yylval.sym = s;
                P("Identificador <%s> (%s / %d)\n",
                    s->name, lookup_type(s->type), s->type);
                add_token(s->type, lineno, saved_col_no, yyleng, lexema);
                return s->type;
            }

            /* NI UNA COSA NI OTRA, no creamos el symbol
             * y devolvemos un token UNDEF */

            const token * t = add_token(UNDEF,
                    lineno, saved_col_no, yyleng, lexema);

            P("Undefined <%s> (%s / %d)\n",
                t->lex, lookup_type(UNDEF), UNDEF);

            yylval.str = t->lex;

            return UNDEF;
        }

\"([^\"\n]|\\.)*\" {
            /* CADENA DE CARACTERES */
            const token *t = add_token(STRING, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
            const char *s;
            yylval.str = intern(s = deescape(yytext));
            free((void *)s);
            P("cadena de caracteres: %s -> [%s]\n",
                t->lex, yylval.str);

            return STRING;
        }
\"([^\"\n]|\\.)*\n {
            /* CADENA DE CARACTERES NO TERMINADA --> ERROR */
            add_token(ERROR, lineno, col_no, yyleng, yytext);
            col_no           = 1;
            return ERROR;
        }

">="    { P("operador %s (%d)\n", yytext, GE);
            add_token(GE, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return GE;  }
"<="    { P("operador %s (%d)\n", yytext, LE);
            add_token(LE, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return LE;  }
"=="    { P("operador %s (%d)\n", yytext, EQ);
            add_token(EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return EQ;  }
"!="    { P("operador %s (%d)\n", yytext, NE);
            add_token(NE, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return NE;  }
"&&"    { P("operador %s (%d)\n", yytext, AND);
            add_token(AND, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return AND; }
"||"    { P("operador %s (%d)\n", yytext, OR);
            add_token(OR, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return OR;  }
"^^"    { P("operador %s (%d)\n", yytext, EXP);
            add_token(EXP, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return EXP; }

"++"    { P("operador %s (%d)\n", yytext, PLS_PLS);
            add_token(EXP, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return PLS_PLS; }

"--"    { P("operador %s (%d)\n", yytext, MIN_MIN);
            add_token(MIN_MIN, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return MIN_MIN; }

"+="    { P("operador %s (%d)\n", yytext, PLS_EQ);
            add_token(PLS_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return PLS_EQ; }

"-="    { P("operador %s (%d)\n", yytext, MIN_EQ);
            add_token(MIN_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return MIN_EQ; }

"*="    { P("operador %s (%d)\n", yytext, MUL_EQ);
            add_token(MUL_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return MUL_EQ; }

"/="    { P("operador %s (%d)\n", yytext, DIV_EQ);
            add_token(DIV_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return DIV_EQ; }

"%="    { P("operador %s (%d)\n", yytext, MOD_EQ);
            add_token(MOD_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return MOD_EQ; }

"^^="   { P("operador %s (%d)\n", yytext, PWR_EQ);
            add_token(PWR_EQ, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
          return PWR_EQ; }

\n      {
            add_token('\n', lineno, col_no, 2, "\\n");
            lineno++;
            col_no = 1;
            P("salto de linea (%d)\n", '\n');
            return '\n';
        }

#.*     ; /* comentario */

.       { P("symbol/operator '%c' (%d)\n",
                yytext[0], yytext[0]);
          add_token(yytext[0], lineno, col_no, 1, yytext);
          col_no += yyleng;
          return yytext[0];
        }

<<EOF>> { P("EOF (0)\n");
          add_token(0, lineno, col_no, 0, "<<EOF>>");
          return 0;
        }

%%

static token  last_tokens_buffer[UQ_LAST_TOKENS_SZ];
static token *last_token        =  last_tokens_buffer + UQ_LAST_TOKENS_SZ;
static int    token_buffer_full = 0;

void reset_add_token(void)
{
    int i;
    for (   i = 0, last_token = last_tokens_buffer;
            i < UQ_LAST_TOKENS_SZ;
            i++, last_token++)
    {
        last_token->tok   = 0;
        last_token->lin   = 0;
        last_token->col   = 0;
        last_token->len   = 0;
        last_token->lex   = NULL;
        last_token->lex   = NULL;
        token_buffer_full = 0;
    }
    col_no = 1;
} /* reset_add_token */


static const token *
add_token(
        int         tok,
        int         lin,
        int         col,
        size_t      len,
        const char *lex)
{
    if (last_token == last_tokens_buffer) {
        last_token        += UQ_LAST_TOKENS_SZ;
        token_buffer_full  = 1;
    }
    last_token--;

    last_token->tok = tok;
    last_token->lin = lin;
    last_token->col = col;
    last_token->len = len;
    last_token->lex = intern(lex);

    return last_token;
} /* add_token */

const token *
get_last_token(
        unsigned pos)
{
    if (pos >= UQ_LAST_TOKENS_SZ)
        return NULL;
    const token *ret_val = last_token + pos;
    if (ret_val >= last_tokens_buffer + UQ_LAST_TOKENS_SZ) {

        if (!token_buffer_full)
            return NULL;

        ret_val -= UQ_LAST_TOKENS_SZ;
    }

    return ret_val;
} /* get_last_token */

FILE *yysetfilename(const char *fn)
{
    FILE *ret_val = fopen(fn, "r");
    if (ret_val) {
        yysetFILE(ret_val);
    }
    return ret_val;
} /* yysetfilename */

void yysetFILE(FILE *in)
{
    yyrestart(in);
    BEGIN INITIAL;
} /* yysetFILE */

static char *
deescape(char *in)
{
    char *ret_val = strdup(in + 1),
         *src     = ret_val,
         *dst     = ret_val;
    assert(ret_val != NULL);
    for (; *src != '"'; src++) {
        char c = *src;
        switch (c) {
            case '\\':
                src++; c = *src;
                switch(c) {
                    case 'a': *dst++ = '\a'; break;
                    case 'b': *dst++ = '\b'; break;
                    case 'e': *dst++ = '\e'; break;
                    case 'f': *dst++ = '\f'; break;
                    case 'n': *dst++ = '\n'; break;
                    case 'r': *dst++ = '\r'; break;
                    case 't': *dst++ = '\t'; break;
                    case 'v': *dst++ = '\v'; break;
                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7': {
                            c = c - '0';
                            P("leemos digito %d\n", c);
                            int i;
                            for (i = 0; i < 2; i++) {
                                src++;
                                if (*src < '0' || *src > '7')
                                    break;
                                P("c(%d) ->", c);
                                c *= 8; c += *src - '0';
                                P(" c(%d)\n", c);
                            }
                            if (i < 2) src--;
                            *dst++ = c;
                            break;
                        }
                    default: *dst++ = *src; break;
                } /* switch */
                break;
            default: *dst++ = *src; break;
        } /* switch */
    } /* for */
    *dst = '\0';
    return ret_val;
} /* deescape */

#if 0
/*  Si esta funcion no se escribe, debemos enlazar con -ll
 *  o poner la opcion %option noyywrap arriba. */
int yywrap()
{
    return 1;
}
#endif
