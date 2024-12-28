/* hoc.h -- tipos y funciones relacionados con la tabla de simbolos.
 * Date: Fri Dec 27 14:57:20 -05 2024
 */

typedef enum sym_type {
    UNDEF,
    VAR,
    BLTIN
} sym_type;

typedef struct Symbol {         /* Symbol table entry */
    const char   *name;
    sym_type      type;         /* VAR, BLTIN, UNDEF */
    union {
        double    val;          /* if VAR */
        double  (*ptr)(double); /* if BLTIN */
    } u;
    struct Symbol *next;        /* link to next */
} Symbol;

Symbol *install(
        const char *name,
        sym_type    typ,
        double      val,
        double    (*ptr)(double));

Symbol *lookup(
        const char *name);
