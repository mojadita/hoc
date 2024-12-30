/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Date: Fri Dec 27 14:57:20 -05 2024
 */

typedef enum sym_type {
    UNDEF,
    VAR,
	BLTIN,
    BLTIN0,
	BLTIN1,
	BLTIN2,
} sym_type;

typedef struct Symbol {                  /* Symbol table entry */
    char         *name;
    sym_type      type;                  /* VAR, BLTIN[012], UNDEF */
    union {
        double    val;                   /* if VAR */
        double  (*ptr0)(void);           /* if BLTIN0 */
        double  (*ptr1)(double);         /* if BLTIN1 */
        double  (*ptr2)(double, double); /* if BLTIN2 */
    } u;
    struct Symbol *next;                 /* link to next */
} Symbol;

Symbol *install(
        const char *name,
        sym_type    typ,
        double      val);

Symbol *lookup(
        const char *name);

void init(void);  /* install constants and built-ins in table */
