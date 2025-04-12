/* instrucciones.h -- juego de instrucciones de la maquina virtual.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:23:22 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

INST(STOP,1)                       /* para la maquina, termina la ejecucion. */
INST(drop,1,)                      /* elimina un valor de la pila */
INST(constpush,2, SUFF(void, datum, prog)) /* introduce un valor constante en la pila */
INST(add,1)                        /* suma los dos valores top de la pila */
INST(sub,1)                        /* resta los dos valores top de la pila Y - X */
INST(mul,1)                        /* multiplica los dos valores top de la pila Y * X */
INST(divi,1)                       /* divide los dos valores top de la pila Y / X */
INST(mod,1)                        /* calcula Y % X */
INST(neg,1)                        /* calcula -X */
INST(pwr,1)                        /* calcula Y ^ X */
INST(eval,2,      SUFF(void, symb, prog))/* evalua una variable */
INST(assign,2,    SUFF(void, symb, prog))/* asigna X a una variable */
INST(print,1)                      /* imprime X */
INST(bltin0,2,     SUFF(void, symb, prog))/* llama a una funcion bltin0 (una sin parametros) */
INST(bltin1,2,     SUFF(void, symb, prog))/* llama a una funcion bltin1 (de un parametro) */
INST(bltin2,2,     SUFF(void, symb, prog))/* llama a una funcion bltin2 (de dos parametros) */
INST(ge,1)                          /* operador Y >= X */
INST(le,1)                          /* operador Y <= X */
INST(gt,1)                          /* operador Y > X */
INST(lt,1)                          /* operador Y < X */
INST(eq,1)                          /* operador Y == X */
INST(ne,1)                          /* operador Y != X */
INST(not,1)                         /* operador ! */
INST(and,1)                         /* operador Y && X (sin cortocircuito) */
INST(and_then,2,   SUFF(void, addr, prog))/* operador Y && X (con cortocircuito) */
INST(or,1)                          /* operador Y || X (sin cortocircuito) */
INST(or_else,2,    SUFF(void, addr, prog))/* operador Y || X (con cortocircuito) */
INST(readopcode,2, SUFF(void, symb, prog))/* lee un valor y lo asigna a una variable */
INST(call,3,   SUFF(void, symb_int, prog))/* llama a una subrutina con los parametros de la pila */
INST(procret,1)                     /* retorna de un procedimiento definido por el usuario */
INST(funcret,1)                     /* retorna un valor de una funcion definida por el usuario */
INST(argeval,2,    SUFF(void, arg, prog)) /* evalua un argumento y lo pone en la pila. */
INST(argassign,2,  SUFF(void, arg, prog)) /* asigna el top de la pila a $n.  X -> $n */
INST(prstr,2,      SUFF(void, str, prog)) /* imprime una cadena */
INST(prexpr,1)                      /* imprime una expresion */
INST(symbs,1)                       /* imprime la tabla de simbolos (desaparecera) */
INST(list,1)                        /* lista el codigo del programa */
INST(if_f_goto,2,  SUFF(void, addr, prog))/* salto si el top de la pila es cero */
INST(Goto,2,       SUFF(void, addr, prog))/* salto incondicional */
INST(noop,1)                        /* no operacion, nada */
