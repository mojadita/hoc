
%{
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

void warning(char *, char *);
int  yylex(void);
void yyerror(char *);
/*  array de variables desde 'a' hasta 'z'  */
double mem[26];        /* memory variables 'a'..'z' */

%}

%union {
    double val;
    int    index;
}

%token <val>   NUMBER
%token <index> VAR
%type  <val>   expr

%right '='     /* right associative, minimum precedence */
%left  '+' '-'  /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY /* new, lo mas todavia */

%%

list: /* nothing */
    | list       '\n'
    | list expr  '\n' {  printf("\t%.8g\n", $2);
                         /* p es asignado en el lugar donde se
                          * imprime un resultado. */
                         mem['p'-'a'] = $2;
			          }
    | list expr ';'   {  /* si se escribe ; entonces hacer salto de linea */
                         printf( "   %.8g\n", $2 );
                         /* p es asignado en el lugar donde se
                          * imprime un resultado. */
                         mem['p'-'a'] = $2;
                      }
    | list error '\n' {  yyerrok;  }
    ;

expr: NUMBER
    | VAR                  { $$ = mem[$1];        }  /*  index  */
    | VAR '=' expr         { $$ = (mem[$1] = $3);
                             /* p es asignado alla donde hay una
                              * asignacion */
                             /* mem['p' - 'a'] = $$; */
                           }  /* asignacion */
    | '-' expr %prec UNARY { $$ = -$2; } /* new */
    | '+' expr %prec UNARY { $$ =  $2; } /* new */
    | '(' expr ')'         { $$ = $2; }
    | expr '%' expr        { $$ = fmod($1, $3); }
    | expr '*' expr        { $$ = $1 * $3; }
    | expr '/' expr        { $$ = $1 / $3; }
    | expr '+' expr        { $$ = $1 + $3; }
    | expr '-' expr        { $$ = $1 - $3; }
    ;

%%

char *progname;     /* for error messages */
int   lineno = 1;   /* numero de linea */

int
main(int argc, char *argv[]) /* hoc1 */
{
    progname = argv[0];
    yyparse();
    return EXIT_SUCCESS;
}

int yylex(void)   /* hoc1 */
{
    int c;

    while ((c=getchar()) == ' ' || c == '\t')
        continue;

    if ( c == 27 )  /*  si se presiona  [Escape] [Enter]  salimos del programa  */
    {
        printf( "  Saliendo .... Chao!!...\n" );
        return 0;
    }
    if (c == EOF)   /*  si se preiona  [Control] d  Salimos del programa  */
        return 0;  /* retornando tipo de token  */
    if (c == '.' || isdigit(c)) { /* number */
        ungetc(c, stdin);  /* retornando tipo de token  */
        if (scanf("%lf", &yylval.val) != 1) {
            //getchar();
            while ((c = getchar()) != EOF && c != '\n')
                continue;
            if (c == '\n')
                ungetc(c, stdin);
            return YYERRCODE;
        }
        return NUMBER;  /* retornando tipo de token  */
    }
    if (islower(c)) {
        yylval.index = c - 'a';
        return VAR;
    }
//    /* si se escribe ; entonces hacer salto de linea */
//    if ( c == ';' )
//         printf( "\n" );
    /*  Salto de linea normal  */
    if (c == '\n')
        lineno++;
    return c;
}

void yyerror(char *s)   /* called for yacc syntax error */
{
    warning(s, NULL);
}

void warning(char *s, char *t)    /* print warning message */
{
    fprintf(stderr, "%s: %s", progname, s);
    if (t)
        fprintf(stderr, " %s", t);
    fprintf(stderr,     " near line %d\n",  lineno);
}
