/* symbol.h -- Estructura e interfaz de la tabla de simbolos
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Wed Jun  4 13:12:10 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef SYMBOL_H /* { */
#define SYMBOL_H

typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;

struct Symbol {                           /* Symbol table entry */
    char          *name;                  /* nombre del simbolo */
    int            type;                  /* tipo del simbolo:
                                           * VAR, BLTIN[012], UNDEF */
    const char    *help;                  /* help text (optional) */
    Symbol        *typref;                /* ref al tipo de la
                                           * variable/func/builtin... */
    union {
        double     val;                   /* si el tipo es CONST */
        double   (*ptr0)(void);           /* si el tipo es BLTIN0 */
        double   (*ptr1)(double);         /* si el tipo es BLTIN1 */
        double   (*ptr2)(double, double); /* si el tipo es BLTIN2 */
        struct {                          /* si el tipo es FUNC, PROC o VAR */
            Cell      *defn;              /* donde empieza el codigo de la funcion */
            Symbol    *prnt_smbl_tble;    /* tabla de symbolos superior */
            /* prototipo de la funcion */
            Symbol    *type_func;         /* tipo devuelto por la funcion */

            /* Datos necesarios para la macro DYNARRAY() */
            Symbol   **argums;            /* puntero a array de punteros a Symbol * */
            size_t     argums_len;        /* longitud del array de Symbol * argums */
            size_t     argums_cap;        /* capacidad del array anterior */

            Cell     **returns_to_patch;  /* lista de returns que hay que parchear */
            size_t     returns_to_patch_len, /* num elementos en la lista */
                       returns_to_patch_cap; /* capacidad de la lista */

            Symbol   **local_scopes;      /* contextos locales de la funcion */
            size_t     local_scopes_len,  /* forman una pila */
                       local_scopes_cap;

            int        nargs;             /* numero de argumentos */
            int        nvars;             /* numero de variables locales */
            size_t     nxt_off,
                       max_off;
        };
        struct {                          /* si el tipo es LVAR */
            int        lv_off;            /* variables locales y argumentos (LVAR),
                                           * offset respecto al frame pointer (fp). */
            Symbol    *proc_func;         /* a que proc/func pertenece este simbolo */
        };
        size_t     size;                  /* si el tipo es TYPE */
    }  /* no hay nombre de campo */ ;
       /* union anonima, el nombre del campo no existe, de forma que los
        * nombres de los campos de la union pueden usarse directamente desde
        * la estructura Symbol.  Esto ***solo*** es valido en C, y no en
        * C++ */
    Symbol        *next;                  /* enlace al siguiente
                                           * simbolo de la tabla.*/
};

SymbolTable *new_symbol_table(void);
void pop_symbol_table(void);

void symbol_table_list(
		const SymbolTable *symtab);
SymbolTable *symbol_table_active(void);

/* instala un nuevo simbolo en la tabla de simbolos, inicializado
 * con el nombre, tipo y valor correspondiente.
 * Los simbolos asociados a funciones BLTIN[012] se inicializan
 * solamente en la funcion init(), con lo que una vez dado un tipo
 * y un nombre, se inicializan alli nada mas.  Los demas se usan
 * en el parser (en diferentes partes) para asignar variables. */
Symbol *install(
        const char *name,
        int         typ);

/* busca un simbolo en la tabla de simbolos. Devuelve NULL si el
 * simbolo no existe. */
Symbol *lookup(
        const char *name);

Symbol *lookup_local(
        const char *name,
        const Symbol *scope);

Symbol *top_symtab(void);

const char *lookup_type(int typ);
void list_symbols(void);
void borrar_scope(Symbol *subr);

#endif /* SYMBOL_H  } */
