%{

#include <assert.h>

#include "config.h"
#include "hoc.h"
#include "hoc.tab.h"
#include "code.h"
#include "reserved_words.h"

#ifndef UQ_LEX_DEBUG
#warning UQ_LEX_DEBUG deberia ser configurado en config.mk
#define UQ_LEX_DEBUG 0
#endif

#if UQ_LEX_DEBUG  /*   {{ */
#define P_TAIL(_fmt, ...)  \
    printf(_fmt, ##__VA_ARGS__)
#define P(_fmt, ...)                      \
    printf("%s:%d:%s "_fmt,               \
            __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__)
#else /* UQ_LEX_DEBUG  }{ */
#define P(_fmt, ...)
#define P_TAIL(_fmt, ...)
#endif /* UQ_LEX_DEBUG }} */

extern int lineno;

/* declaracion adelantada */
static char *deescape(char *in);

%}

%option noyywrap
%x COMMENT

%%


[ \t]    ;  /* saltar blancos y tabuladores */

([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
            sscanf(yytext, "%lf", &yylval.val);
            P("number %g (%d)\n", yylval.val, NUMBER);
            return NUMBER;
        }   /* Un numero punto flotante  */

"/*"    {   BEGIN COMMENT;
            P("%s", yytext);
        }
<COMMENT>. { P_TAIL("%s", yytext); }
<COMMENT>\n { P_TAIL("\n"); P(""); }
<COMMENT>"*/" {
            P_TAIL("%s\n", yytext);
            BEGIN INITIAL;
        }

[a-zA-Z_][a-zA-Z0-9_]*  {
            const reserved_word *rw = rw_lookup(yytext);
            if (rw) {
                P("reserved word '%s' -> %s(%d)\n",
                    rw->name, rw->tokn_str, rw->tokn);
                return rw->tokn;
            }
            Symbol *s;
            if ((s = lookup(yytext)) == NULL) {
                /* no esta, lo instalamos */
                s = install(yytext, UNDEF, 0.0, NULL);
            }
            yylval.sym = s;
            P("Symbol <%s> (%d)\n", s->name, s->type);
            return s->type == UNDEF ? VAR : s->type;
        }
"$"[1-9][0-9]*  {
            sscanf(yytext+1, "%d", &yylval.num);
            P("nombre de parametro: %s\n", yytext);
            return ARG;
        }

\"([^\"\n]|\\.)*\" {
            yylval.str = deescape(yytext);
            P("cadena de caracteres: %s -> [%s]\n",
                yytext, yylval.str);
            return STRING;
        }
\"([^\"\n]|\\.)*\n {
            execerror("cadena de caracteres no terminada: %s",
                yytext);
        }

">="    { P("operador %s (%d)\n", yytext, GE); return GE; }
"<="    { P("operador %s (%d)\n", yytext, LE); return LE; }
"=="    { P("operador %s (%d)\n", yytext, EQ); return EQ; }
"!="    { P("operador %s (%d)\n", yytext, NE); return NE; }
"&&"    { P("operador %s (%d)\n", yytext, AND); return AND; }
"||"    { P("operador %s (%d)\n", yytext, OR); return OR; }

\n      {   lineno++;
            P("salto de linea (%d)\n", '\n');
            return '\n';
        }

#.*     ;

.       { P("symbol/operator '%c' (%d)\n",
                yytext[0], yytext[0]);
          return yytext[0]; }

<<EOF>> { P("EOF (0)\n"); return 0; }

%%

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
