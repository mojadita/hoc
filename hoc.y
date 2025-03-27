/* area de definiciones y configuracion de yacc
 * se extiende hasta encontrar %% aislado en una linea */
/* hoc.y -- analizador sintactico de hoc.
 * Date: Mon Dec 30 11:59:54 -05 2024
 */
%{
/* area reservada para insertar codigo C */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "config.h"
#include "colors.h"
#include "hoc.h"
#include "error.h"
#include "math.h"   /*  Modulo personalizado con nuevas funciones */
#include "instr.h"
#include "code.h"

static void yyerror(char *);

/*  Necesario para hacer setjmp y longjmp */
jmp_buf begin;

#ifndef  UQ_HOC_DEBUG
#warning UQ_HOC_DEBUG deberia ser configurado en config.mk
#define  UQ_HOC_DEBUG 1
#endif

#ifndef  UQ_HOC_TRACE_PATCHING
#warning UQ_HOC_TRACE_PATCHING deberia ser configurado en config.mk
#define  UQ_HOC_TRACE_PATCHING 1
#endif

#if       UQ_HOC_DEBUG /* {{ */
# define P(_fmt, ...)                     \
    printf("%s:%d:%s "_fmt,               \
            __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__)
#else  /* UQ_HOC_DEBUG    }{ */
# define P(_fmt, ...)
#endif /* UQ_HOC_DEBUG    }} */

#if UQ_HOC_TRACE_PATCHING
# define PT(_fmt, ...) P(_fmt, ##__VA_ARGS__)
#else
# define PT(_fmt, ...)
#endif

#define CODE_INST(I) code_inst(INST_##I)
#define CODE_STOP()  CODE_INST(STOP)

int indef_proc,  /* 1 si estamos en una definicion de procedimiento */
    indef_func;  /* 1 si estamos en una definicion de funcion */

%}
/* continuamos el area de definicion y configuracion
 * de yacc */

/*  Declaracion tipos de datos de los objetos
    (TOKENS, SYMBOLOS no terminales)  */
%union {
    const instr *inst; /* machine instruction */
    Symbol      *sym;  /* symbol table pointer */
    double       val;  /* double value */
    Cell        *cel;  /* Cell reference */
    int          num;  /* valor entero, para $<n> */
    char        *str;  /* cadena de caracteres */
}

/* Los valores que retorna la fncion  yylex son declarados con
 * la directiva %token.
 *
 * LCU: Tue Jan 21 10:55:22 EET 2025
 * definimos los mismos tokens aqui que en hoc-sin-prec.y, para
 * asegurar que se asignaran los mismos valores, y no habra
 * problemas al construir ambos ejecutables.
 */
%token ERROR
%token <val> NUMBER
%token <sym> VAR BLTIN0 BLTIN1 BLTIN2 UNDEF CONST
%token       PRINT WHILE IF ELSE SYMBS
%token       OR AND GE LE EQ NE
%token       UNARY
%token <num> ARG
%token <int> FUNC PROC
%token       RETURN READ
%token <sym> FUNCTION PROCEDURE
%token <str> STRING
%token       LIST
%type  <cel> stmt asig expr stmtlist cond mark
%type  <cel> expr_seq item do else and or
%type  <num> arglist_opt arglist

/* la directiva %type indica los tipos de datos de los diferentes
 * simbolos no terminales, definidos en la gramatica */

%right '='         /* right associative, minimum precedence */
%right OR           /* || */
%right AND          /* && */
%left '!'
%left  '>' GE '<' LE EQ NE /* > >= <= == != */
%left  '+' '-'     /* left associative, same precedence */
%left  '*' '/' '%' /* left associative, higher precedence */
%left  UNARY       /* new, lo mas todavia */
%right '^'         /* operador exponenciacion */

/* fin del area de configuracion de yacc */
%%
/*  Area de definicion de reglas gramaticales */

list: /* nothing */
    | list       '\n'
    | list defn  '\n'
    | list stmt  '\n'      { CODE_STOP();
                             return 1; }
    | list asig  '\n'      { CODE_INST(assign);
                             code_sym(lookup("prev"));
                             CODE_INST(print);
                             CODE_STOP();
                             return 1; }
    | list error final     { yyerrok;
                             return 1; }
    ;

final: '\n' | ';' ;

stmt: asig        ';'      { CODE_INST(drop); }
    | RETURN      ';'      { defnonly(indef_proc, "return");
                             $$ = CODE_INST(procret); }
    | RETURN expr ';'      { defnonly(indef_func, "return <expr>");
                             $$ = $2;
                             CODE_INST(funcret); }
    | PRINT  expr_seq ';'  { $$ = $2; }
    | SYMBS       ';'      { $$ = CODE_INST(symbs); }
    | LIST        ';'      { $$ = CODE_INST(list); }
    | WHILE cond do stmt   { CODE_INST(Goto);
                             code_cel($2);
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> begin patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto);
                                 code_cel(saved_progp);
                             PT("<<< end   patching CODE @ [%04lx], "
                                "continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp; }
    | IF    cond do stmt   { Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto);
                                 code_cel(saved_progp);
                             PT("<<< end patching CODE @ [%04lx], continuing @ [%04lx]\n",
                                 progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | IF    cond do stmt else stmt {
                             Cell *saved_progp = progp;
                             progp = $3;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(if_f_goto);
                                 code_cel($6);
                             PT("<<< end patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = $5;
                             PT(">>> patching CODE @ [%04lx]\n", progp - prog);
                                 CODE_INST(Goto);
                                 code_cel(saved_progp);
                             PT("<<< end patching CODE @ [%04lx], "
                                   "continuing @ [%04lx]\n",
                                   progp - prog, saved_progp - prog);
                             progp = saved_progp;
                           }
    | '{' stmtlist '}'     { $$ = $2; }
    | PROCEDURE mark '(' arglist_opt ')' ';' {
                             $$ = $2;
							 CODE_INST(call); /* instruction */
                             code_sym($1);    /* symbol associated to proc*/
                             code_num($4);    /* number of arguments */ }
    ;

do  :  /* empty */         { $$ = progp;
                             PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog);
                                 CODE_INST(if_f_goto);
                                 code_cel(NULL);
                             PT("<<< end   inserting unpatched CODE\n"); }
    ;

else:  ELSE                { $$ = progp;
                             PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                     progp - prog);
                                 CODE_INST(Goto);
                                 code_cel(NULL);
                             PT("<<< end   inserting unpatched CODE\n"); }
    ;

expr_seq
    : expr_seq ',' item
    | item
    ;

item: STRING               { $$ = CODE_INST(prstr);
                             code_str($1); }
    | asig                 { CODE_INST(prexpr); }
    ;


cond: '(' asig ')'         { $$ = $2; }
    ;

mark: /* empty */          { $$ = progp; }
    ;

stmtlist: /* empty */      { $$ = progp; }
    | stmtlist '\n'
    | stmtlist stmt
    ;

asig: VAR   '=' asig       { if ($1->type != VAR && $1->type != UNDEF) {
                                 execerror("symbol '%s' is not a variable\n",
                                           $1->name);
                             }
                             $$ = $3;
                             CODE_INST(assign);
                             code_sym($1); }
    | ARG   '=' asig       { defnonly(indef_proc || indef_func, "$%d assign", $1);
                             $$ = $3;
                             CODE_INST(argassign);
                             code_num($1);
                           }
    | expr
    ;

expr: NUMBER                        { $$ = CODE_INST(constpush);
                                           code_val($1); }
    | VAR                           { $$ = CODE_INST(eval);
                                           code_sym($1); }
    | ARG                           { defnonly(indef_proc || indef_func,
                                                "$%d assign", $1);
                                      $$ = CODE_INST(argeval);
                                           code_num($1);
                                    }

    | CONST                         { CODE_INST(eval);
                                      code_sym($1); }
    | BLTIN0 '(' ')'                { $$ = CODE_INST(bltin0);
                                           code_sym($1); }
    | BLTIN1 '(' expr ')'           { $$ = $3;
                                      CODE_INST(bltin1);
                                      code_sym($1); }
    | BLTIN2 '(' expr ',' expr ')'  { $$ = $3;
                                      CODE_INST(bltin2);
                                      code_sym($1); }
    | expr '>' expr                 { CODE_INST(gt);  }
    | expr '<' expr                 { CODE_INST(lt);  }
    | expr EQ  expr  /* == */       { CODE_INST(eq);  }
    | expr NE  expr  /* != */       { CODE_INST(ne);  }
    | expr GE  expr  /* >= */       { CODE_INST(ge);  }
    | expr LE  expr  /* <= */       { CODE_INST(le);  }
    | expr and expr  /* && */       { Cell *saved_progp = progp;
                                      progp = $2;
                                      PT(">>> begin patching CODE @ [%04lx]\n",
                                            progp - prog);
                                          CODE_INST(and_then);
                                          code_cel(saved_progp);
                                      PT("<<< end   patching CODE @ [%04lx], "
                                            "continuing @ [%04lx]\n",
                                            progp - prog, saved_progp - prog);
                                      progp = saved_progp; }
    | expr or  expr  /* || */       { Cell *saved_progp = progp;
                                      progp = $2;
                                      PT(">>> begin patching CODE @ [%04lx]\n",
                                            progp - prog);
                                          CODE_INST(or_else);
                                          code_cel(saved_progp);
                                      PT("<<< end   patching CODE @ [%04lx], "
                                            "continuing @ [%04lx]\n",
                                            progp - prog, saved_progp - prog);
                                      progp = saved_progp; }
    | expr '+' expr                 { CODE_INST(add); }
    | expr '-' expr                 { CODE_INST(sub); }
    | expr '%' expr                 { CODE_INST(mod); }
    | expr '*' expr                 { CODE_INST(mul); }
    | expr '/' expr                 { CODE_INST(divi); }
    | '(' expr ')'                  { $$ = $2; }
    | expr '^' expr                 { CODE_INST(pwr); }
    | '+' expr %prec UNARY          { $$ = $2; }
    | '-' expr %prec UNARY          { CODE_INST(neg); $$ = $2; } /* new */
    | '!' expr %prec UNARY          { CODE_INST(not); $$ = $2; } /* not */
    | READ '(' VAR ')'              { $$ = CODE_INST(readopcode);
                                           code_sym($3); }
    | FUNCTION mark '(' arglist_opt ')' {
                                      $$ = $2;
                                      CODE_INST(call); /* instruction */
                                      code_sym($1);    /* function symbol */
                                      code_num($4);    /* number of arguments */ }
    ;

and : AND                           { PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                          progp - prog);
                                      $$ = CODE_INST(and_then);
                                           code_cel(NULL);
                                      PT("<<< end   inserting unpatched CODE\n"); }
    ;

or  : OR                            { PT(">>> begin inserting unpatched CODE @ [%04lx]\n",
                                         progp - prog);
                                      $$ = CODE_INST(or_else);
                                           code_cel(NULL);
                                      PT("<<< end   inserting unpatched CODE\n"); }
    ;

arglist_opt
    : arglist
    | /* empty */           { $$ = 0; }
    ;

arglist
    : arglist ',' asig      { $$ = $1 + 1; }
    | asig                  { $$ = 1; }
    ;

defn: proc_head '(' ')' stmt {
                              CODE_INST(procret);
                              end_define();
                              indef_proc    = 0;
                              P("FIN DEFINICION PROCEDIMIENTO\n");
                            }
    | func_head '(' ')' stmt {
                              CODE_INST(constpush);
                              code_val(0.0);
                              CODE_INST(funcret);
                              end_define();
                              indef_func    = 0;
                              P("FIN DEFINICION FUNCION\n");
                            }
proc_head
    : PROC VAR              {
                              P("DEFINIENDO EL PROCEDIMIENTO '%s' @ [%04lx]\n",
                                $2->name, progp - prog);
                              define($2, PROCEDURE);
                              indef_proc    = 1; }
func_head
    : FUNC VAR              {
                              P("DEFINIENDO LA FUNCION '%s' @ [%04lx]\n",
                                $2->name, progp - prog);
                              define($2, FUNCTION);
                              indef_func    = 1; }

/* Fin Area de definicion de reglas gramaticales */
%%

static void yyerror(char *s)   /* called for yacc syntax error */
{
    /* LCU: Wed Mar 19 15:40:42 -05 2025
     * TODO: manejar un buffer circular de tokens que permita
     * imprimir mejor el contexto donde ocurren los errores 
     * de la gramatica. */
    //warning("%s", s);
    warning(F(" "BRIGHT GREEN"%s"), s);
} /* yyerror */
