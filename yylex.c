/* yylex.c -- la funcion yylex original ha sido movida a un
 * fichero aparte.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Jan 11 13:11:00 -05 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD.
 */

#include <stdio.h>
#include <ctype.h>

#include "hoc.h"
#include "hoc.tab.h"

extern int lineno;

static int follow(int expected, int ifyes, int ifno)
{
    int c = fgetc(stdin);
    if (c == EOF)
        return 0;
    if (c == expected)
        return ifyes;
    ungetc(c, stdin);
    return ifno;
} /* follow */

/*  Esta funcion produce un TOKEN  */
int
yylex(void)   /* hoc1 */
{
    int c;

    while ((c=getchar()) == ' ' || c == '\t')
        continue;
    /*  c != ' ' && c != '\t' */

    /*  si se presiona  [Escape] [Enter]  salimos del programa  */
    /*  [Escape]:   \e   27    x01b   */
    if ( c == '\033' ) {
        printf( "  Saliendo .... Chao!!...\n" );
        return 0;
    }
    if (c == EOF)  /* si se preiona Cntrl-d Salimos del programa*/
        return 0;  /* retornando tipo de token  */

    if (c == '.' || isdigit(c)) { /* number */
        ungetc(c, stdin);  /* retornando tipo de token  */
        /* el valor que se usa en el interprete, se coloca en
         * la variable yylval (se usa el campo correspondiente
         * de la union de acuerdo al tipo declarado para el token
         * en la directiva %token arriba. */
        double d;
        if (scanf("%lf", &d) != 1) {
#if 1
            /*  Esta es la condicion que sempre se cumple  */
            /*  Leer un solo caracter  */
            getchar();  /*  esto es similar al codigo de abajo  */
#else
            /*  Lectura hasta el final de la linea  */
            while ((c = getchar()) != EOF && c != '\n')
                continue;
            /* c == EOF || c == '\n' */

            /*  Se hace para que luego el parser lea el siguiente TOKENs
             * (si c diera la casualidad de ser un salto de linea '\n' */
            if (c == '\n')
                ungetc(c, stdin);  /* retrocede el ulitmo caracter leido */
#endif
            return ERROR;
        }
        yylval.val = d;
        return NUMBER;  /* retornando tipo de token  */
    }
    if (isalpha(c)) {
        Symbol *s;
        char sbuf[100], *p = sbuf;

        do {
            *p++ = c;
        } while (((c = getchar()) != EOF) && isalnum(c));
        /* c == EOF || !isalnum(c) */
        if (c != EOF) ungetc(c, stdin);
        *p = '\0';
        if ((s = lookup(sbuf)) == NULL)
            s = install(sbuf, UNDEF, NULL);
        /* el valor que se usa en el interprete */
        yylval.sym = s;
        return s->type == UNDEF
                ? VAR
                : s->type;
    }
    switch(c) {
    case '>': return follow('=', GE,  '>');
    case '<': return follow('=', LE,  '<');
    case '=': return follow('=', EQ,  '=');
    case '!': return follow('=', NE,  '!');
    case '|': return follow('|', OR,  '|');
    case '&': return follow('&', AND, '&');
    case '\n': lineno++; return '\n';
    }
    return c;
} /* yylex */
