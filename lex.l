%{
/* lex.l -- scanner lexico para hoc.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Apr  5 13:31:10 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <assert.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "code.h"
#include "lex.h"
#include "reserved_words.h"

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
         printf(_fmt, ##__VA_ARGS__)
# define P(     _fmt, ...) \
         printf(_fmt, ##__VA_ARGS__)

#else /* UQ_LEX_DEBUG  }{ */

# define P(       _fmt, ...)
# define P_TAIL(  _fmt, ...)

#endif /* UQ_LEX_DEBUG }} */

#if       UQ_LEX_COMMENTS /* {{ */
# define C_TAIL(_fmt, ...)  \
         printf(_fmt, ##__VA_ARGS__)
# define C(     _fmt, ...)  \
         printf(_fmt, ##__VA_ARGS__)
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

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
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
            /* IDENTIFICADOR O PALABRA RESERVADA */
            int saved_col_no = col_no;
            col_no          += yyleng;
            const reserved_word *rw = rw_lookup(yytext);
            if (rw) {
                P("reserved word '%s' -> %s(%d)\n",
                    rw->name, rw->tokn_str, rw->tokn);
                add_token(rw->tokn, lineno, saved_col_no, yyleng, yytext);

                return rw->tokn;
            }
            Symbol *s;
            if ((s = lookup(yytext)) == NULL) {
                /* no esta, lo instalamos */
                s = install(yytext, UNDEF, 0.0, NULL);
            }
            yylval.sym = s;
            P("Symbol <%s> (%d)\n", s->name, s->type);
            int saved_token_type = s->type == UNDEF ? VAR : s->type;
            add_token(saved_token_type, lineno, saved_col_no, yyleng, yytext);

            return saved_token_type;
        }
\$[1-9][0-9]*  {
            /* ARGUMENTO DE PROCEDIMIENTO/FUNCION */
            sscanf(yytext+1, "%d", &yylval.num);
            P("nombre de parametro: %s\n", yytext);
            add_token(ARG, lineno, col_no, yyleng, yytext);
            col_no += yyleng;

            return ARG;
        }

\"([^\"\n]|\\.)*\" {
            /* CADENA DE CARACTERES */
            add_token(STRING, lineno, col_no, yyleng, yytext);
            col_no += yyleng;
            yylval.str = deescape(yytext);
            P("cadena de caracteres: %s -> [%s]\n",
                yytext, yylval.str);

            return STRING;
        }
\"([^\"\n]|\\.)*\n {
            /* CADENA DE CARACTERES NO TERMINADA --> ERROR */
            int saved_lineno = lineno++,
                saved_col_no = col_no;
            col_no           = 1;
            execerror("cadena no terminada en linea %d, col %d: %s",
                saved_lineno, saved_col_no, yytext);
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

\n      {   
            add_token('\n', lineno, col_no, 2, "\\n");
            lineno++;
            col_no = 1;
            P("salto de linea (%d)\n", '\n');
            return '\n';
        }

#.*     ;

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
        last_token->tok = 0;
        last_token->lin = 0;
        last_token->col = 0;
        last_token->len = 0;
        if (last_token->lex) {
            free((void *)last_token->lex);
        }
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
    if (last_token->lex != NULL) {
        free((void *)last_token->lex);
    }
    last_token->lex = strdup(lex);

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
