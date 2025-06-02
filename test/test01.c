/*
    test01.c
    $   cc test01.c hash.c -o test01.c
*/

#include <stdio.h>
#include <string.h>

#include "hash.h"

int hash(const char *s)
{
    int c, ret_val=119;
    while ((c = *s++) != 0) {
        ret_val *= 23;
        ret_val += c;
        ret_val %= 1337;
    }
    return ret_val;
}

struct valores {
    char *nombre;
    long  valor;
} tabla[] = {
    { "pasifjapfj",                                  0 },
    { "aspdfjaspodfijapsodjfca",                     1 },
    { "aspdfapodjf psoip nfcpas nj",                 2 },
    { "asdfj aspjf asjfd apsdjf aposj f",            3 },
    { "paadijfpoas psj nspj fnsapjnf asjfpasjf nb",  4 },
    { "psdajfpasoidjf",                              5 },
    { "sdpajvoamvnvapsuh",                           6 },
    { "aspdfapodjf psoip nfcpas nj", /* repe */      7 },
    { "apasdiojfjvfdpjnsadpj[fmc i",                 8 },
    { "pasi fjcpdsjfpoispjmaspdfj",                  9 },
    { NULL, },
};

int main()
{

    /*  Prueba Generar Hashes  */
    printf( "  /* Prueba Generar Hashes */\n" );
    for (struct valores *p = tabla; p->nombre; ++p) {
        printf("  %s -> %d\n", p->nombre, hash(p->nombre));
    }
    printf( "\n" );

    /*  [1]  Crear Tabla de Hashes  */
    printf( "  /* [1] Crear Tabla de Hashes *\n" );
    struct hash_map *hash_table = NULL;
    hash_table = new_hash_map( 79,
                               hash,        /* funcion hash */
                               strcmp );    /* funcion de comparacion (equals) */
    printf( "\n" );

    /*  [2] Cargar la Tabla de Hashes  */
    printf( "  /* [2] Cargar Tabla de Hashes */\n" );
    for (struct valores *p = tabla; p->nombre; ++p) {
        struct pair *q = hash_map_put(hash_table, p->nombre, (void*) p->valor);
        if (q) {
            printf("  %ld\t:%s\n", (long)q->val, q->key);
        } else {
            printf("_ Value for %s, couldn't be inserted\n", p->nombre);
        }
    }
    printf( "\n" );

    /*  [3] Consultar un Registro de la Tabla de Hashes  */
    printf( "  /* [3] Consultar un Registro de Tabla de Hashes */\n" );
    char *key = "aspdfapodjf psoip nfcpas nj";
    long result = (long) hash_map_get( hash_table, key);
    printf( "  %s -> %ld\n", key, result);
    printf( "\n" );

    /*  [4] Consltar el Numero de Registros o Entreadas de la tabla Hash  */
    printf( "  /* [4] Consultar el numero de entradas en la Tabla de Hashes */\n" );
    printf( "  mapsize: %zi elements\n", hash_map_size(hash_table));
    printf( "\n" );

    /*  [5]  Borrar la Tabla Hash  */
    printf( "  /* [5] Borrar la Tabla de Hashes */\n" );
    del_hash_map( hash_table );
    printf( "\n" );

    return 0;
}
