/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Date: Fri Dec 27 14:57:20 -05 2024
 */
#ifndef HOC_H
#define HOC_H

#include <setjmp.h>

#define OUTPUT_FMT   "%32.8g"

typedef struct Symbol {                   /* Symbol table entry */
    char          *name;                  /* nombre del simbolo */
    int            type;                  /* tipo del simbolo:
                                           * VAR, BLTIN[012], UNDEF */
	const char    *help;                  /* help text (optional) */
    union {
        double     val;                   /* si el tipo es VAR */
        double   (*ptr0)(void);           /* si el tipo es BLTIN0 */
        double   (*ptr1)(double);         /* si el tipo es BLTIN1 */
        double   (*ptr2)(double, double); /* si el tipo es BLTIN2 */
    }  /* no hay nombre de campo */ ;
	   /* union anonima, el nombre del campo no existe, de forma que los
		* nombres de los campos de la union pueden usarse directamente desde
		* la estructura Symbol.  Esto ***solo*** es valido en C, y no en 
		* C++ */
    struct Symbol *next;                  /* enlace al siguiente
                                           * simbolo de la tabla.*/
} Symbol;

/* instala un nuevo simbolo en la tabla de simbolos, inicializado
 * con el nombre, tipo y valor correspondiente.
 * Los simbolos asociados a funciones BLTIN[012] se inicializan
 * solamente en la funcion init(), con lo que una vez dado un tipo
 * y un nombre, se inicializan alli nada mas.  Los demas se usan
 * en el parser (en diferentes partes) para asignar variables. */
Symbol *install(
        const char *name,
        int         typ,
        double      val,
		const char *help);

/* busca un simbolo en la tabla de simbolos. Devuelve NULL si el
 * simbolo no existe. */
Symbol *lookup(
        const char *name);

void list_symbols(void);

/* inicializa la tabla de simbolos con las funciones builtin y las
 * variables predefinidas. */
void init(void);  /* install constants and built-ins in table */
void execerror(const char *fmt, ...);
int yylex(void);

extern jmp_buf begin;
extern int lineno;
extern char *progname;

typedef double Datum;

extern Datum pop(void);

typedef void (*Inst)(void); /* machine instruction */

#define STOP (Inst) 0

typedef union Cell Cell;

/*  Celda de Memoria RAM donde se instala el programa  */
union Cell {
	Inst    inst;
	Symbol *sym;
	double  val;
	Cell   *cel;
};

extern Cell prog[];

#endif /* HOC_H */
