%{
/* lex.l -- scanner lexico para hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Apr  5 13:31:10 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

#include <assert.h>
#include <ctype.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "code.h"
#include "lex.h"
#include "reserved_words.h"
#include "scope.h"
#include "intern.h"
#include "symbolP.h"
#include "cellP.h"

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

static token *
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

hex         (0[xX][0-9a-fA-F]*)
oct         (0[0-7]*)
dec         ([1-9][0-9]*)
dbl         (([0-9]+\.|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?)
chr         ([^\n\\]|\\[\\\"\'abfnrtv]|\\[0-7]{1,3}|\\[xX][0-9a-fA-F]{2})

suf         [hHlL]?
flt         [fF]?
sufd        {flt}?

%option noyywrap
%x COMMENT

%%

" "+          {  col_no += yyleng; /* saltar blancos y tabuladores */ }
\t            {  col_no += UQ_TAB_SIZE; col_no -= col_no % UQ_TAB_SIZE; }
\\\n          {  col_no = 1; lineno++; }

{hex}{suf}    {

                 /* numero entero hex */
#define INTEGER_BASE(_base)  do {                                         \
                    char *p;                                              \
                    int   tok = INTEGER;                                  \
                    Cell  lit;                                            \
                                                                          \
                    /* OJO, p se inicializa en esta llamada */            \
                    lit.num = strtoul(yytext, &p, _base);                 \
                                                                          \
                    /* comprobamos el siguiente caracter tras literal */  \
                    switch (toupper(*p)) {                                \
                                                                          \
                    case 'L':                                             \
                        P("long " FMT_LONG " (%d)\n", lit.num, LONG);     \
                        tok = LONG; break;                                \
                                                                          \
                    case 'H':                                             \
                        lit.sht = lit.num;                                \
                        P("short " FMT_SHORT " (%d)\n", lit.sht, SHORT);  \
                        tok = SHORT; break;                               \
                                                                          \
                    default:                                              \
                        lit.inum = lit.num;                               \
                        P("integer " FMT_INT " (%d)\n", lit.inum, INTEGER);\
                        break;                                            \
                    }                                                     \
                                                                          \
                    token *t = add_token(                                 \
                            tok, lineno, col_no, yyleng, yytext);         \
                    col_no += yyleng;                                     \
                                                                          \
                                                                          \
                    yylval.lit = lit;                                     \
                    return tok;                                           \
                                                                          \
                } while (0)

                INTEGER_BASE(16);

              }   /* Un numero integer (hex) */

{oct}{suf}  {
                INTEGER_BASE(8);
              }   /* Un numero integer (octal) */

{dec}{suf}  {
                INTEGER_BASE(10);
              }   /* Un numero integer (decimal) */
'{chr}'     {
                char *p   = yytext + 1;
                int   c;
                int dat   = 0;

                switch (c = *p++) {
                case '\\':  /* estado IN_BACKSLASH */
                    switch (c = *p++) {
                    case '\"': dat = '\"'; break;
                    case '\'': dat = '\''; break;
                    case '\\': dat = '\\'; break;
                    case 'a':  dat = '\a'; break;
                    case 'b':  dat = '\b'; break;
                    case 'f':  dat = '\f'; break;
                    case 'n':  dat = '\n'; break;
                    case 'r':  dat = '\r'; break;
                    case 't':  dat = '\t'; break;
                    case 'v':  dat = '\v'; break;
                    case '0': case '1': case '2': case '3': /* \ooo octal */
                    case '4': case '5': case '6': case '7':
                        dat = c - '0';
                        for (int i = 0; i < 2; i++) {
                            switch(c = *p++) {
                            case '0': case '1': case '2': case '3':
                            case '4': case '5': case '6': case '7':
                                dat *= 8;
                                dat += c - '0';
                                continue;
                            } /* switch */
                            break;
                        } /* for */
                        break;
                    case 'x':                               /* \xhh hex */
                        for (int i = 0; i < 2; i++) {
                            switch (c = tolower(*p++)) {
                            case '0': case '1': case '2': case '3':
                            case '4': case '5': case '6': case '7':
                            case '8': case '9':
                                dat *= 16;
                                dat += c - '0';
                                break;
                            case 'a': case 'b':
                            case 'c': case 'd': case 'e': case 'f':
                                dat <<= 4;
                                dat |= c - 'a' + 10;
                                break;
                           } /* switch */
                        } /* for */
                        break;
                    } /* switch */
                    break;
                default:
                    dat = c;
                } /* switch */

                P("char 0x%02x (%d)\n", dat, CHAR);

                add_token(CHAR, lineno, col_no, yyleng, yytext);
                col_no += yyleng;

                yylval.lit.chr = dat;

                return CHAR;
            }

{dbl}{sufd} {
                /* numero en punto flotante */
                char *p;
                int   tok = DOUBLE;

                double lit = strtod(yytext, &p);

                switch (toupper(*p)) {
                case 'F':
                    yylval.lit.flt = lit;
                    tok = FLOAT;
                    P("float " FMT_FLOAT " (%d)\n", yylval.lit.flt, tok);
                    break;

                default:
                    yylval.lit.val = lit;
                    P("double " FMT_DOUBLE " (%d)\n", yylval.lit.val, tok);
                    break;
                }

                add_token(tok, lineno, col_no, yyleng, yytext);
                col_no += yyleng;

                return tok;

              } /* Un numero punto flotante  */

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
          yylval.tok  = *add_token(GE, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return GE;  }
"<="    { P("operador %s (%d)\n", yytext, LE);
          yylval.tok  = *add_token(LE, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return LE;  }
"=="    { P("operador %s (%d)\n", yytext, EQ);
          yylval.tok  = *add_token(EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return EQ;  }
"!="    { P("operador %s (%d)\n", yytext, NE);
          yylval.tok  = *add_token(NE, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return NE;  }
"&&"    { P("operador %s (%d)\n", yytext, AND);
          yylval.tok  = *add_token(AND, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return AND; }
"||"    { P("operador %s (%d)\n", yytext, OR);
          yylval.tok  = *add_token(OR, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return OR;  }
"^^"    { P("operador %s (%d)\n", yytext, EXP);
          yylval.tok  = *add_token(EXP, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return EXP; }

"++"    { P("operador %s (%d)\n", yytext, PLS_PLS);
          yylval.tok  = *add_token(EXP, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return PLS_PLS; }

"--"    { P("operador %s (%d)\n", yytext, MIN_MIN);
          yylval.tok  = *add_token(MIN_MIN, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return MIN_MIN; }

"+="    { P("operador %s (%d)\n", yytext, PLS_EQ);
          yylval.tok  = *add_token(PLS_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return PLS_EQ; }

"-="    { P("operador %s (%d)\n", yytext, MIN_EQ);
          yylval.tok  = *add_token(MIN_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return MIN_EQ; }

"*="    { P("operador %s (%d)\n", yytext, MUL_EQ);
          yylval.tok  = *add_token(MUL_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return MUL_EQ; }

"/="    { P("operador %s (%d)\n", yytext, DIV_EQ);
          yylval.tok  = *add_token(DIV_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return DIV_EQ; }

"%="    { P("operador %s (%d)\n", yytext, MOD_EQ);
          yylval.tok  = *add_token(MOD_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return MOD_EQ; }

"^^="   { P("operador %s (%d)\n", yytext, PWR_EQ);
          yylval.tok  = *add_token(PWR_EQ, lineno, col_no, yyleng, yytext);
          col_no     += yyleng;
          return PWR_EQ; }

\n      {
            add_token('\n', lineno, col_no, 2, "\\n");
            lineno++;
            col_no = 1;
            P("salto de linea (%d)\n", '\n');
            return '\n';
        }

#.*     ; /* comentario */

.       { P("symbol/operator '%c' (%d)\n", yytext[0], yytext[0]);

          yylval.tok  = *add_token(yytext[0], lineno, col_no, 1, yytext);
          col_no     += yyleng;

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
        last_token->id    = 0;
        last_token->lin   = 0;
        last_token->col   = 0;
        last_token->len   = 0;
        last_token->lex   = NULL;
        last_token->lex   = NULL;
        token_buffer_full = 0;
    }
    col_no = 1;
} /* reset_add_token */


static token *
add_token(
        int         id,
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

    last_token->id  = id;
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
