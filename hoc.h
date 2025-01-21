/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Date: Fri Dec 27 14:57:20 -05 2024
 */

#include <setjmp.h>

#define OUTPUT_FMT   "%32.8g"

typedef struct Symbol {                   /* Symbol table entry */
    char          *name;                  /* nombre del simbolo */
    int            type;                  /* tipo del simbolo:
                                           * VAR, BLTIN[012], UNDEF */
    union {
        double     val;                   /* si el tipo es VAR */
        double   (*ptr0)(void);           /* si el tipo es BLTIN0 */
        double   (*ptr1)(double);         /* si el tipo es BLTIN1 */
        double   (*ptr2)(double, double); /* si el tipo es BLTIN2 */
    } u;
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
        double      val);

/* busca un simbolo en la tabla de simbolos. Devuelve NULL si el
 * simbolo no existe. */
Symbol *lookup(
        const char *name);

/* inicializa la tabla de simbolos con las funciones builtin y las
 * variables predefinidas. */
void init(void);  /* install constants and built-ins in table */
void execerror(const char *fmt, ...);
int yylex(void);

extern jmp_buf begin;
extern int lineno;
extern char *progname;
