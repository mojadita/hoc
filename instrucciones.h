/* instrucciones.h -- juego de instrucciones de la maquina virtual.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:23:22 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */

INST(STOP)          /* para la maquina, termina la ejecucion. */
INST(drop)          /* elimina un valor de la pila */
INST(constpush)     /* introduce un valor constante en la pila */
INST(add)           /* suma los dos valores top de la pila */
INST(sub)           /* resta los dos valores top de la pila Y - X */
INST(mul)           /* multiplica los dos valores top de la pila Y * X */
INST(divi)          /* divide los dos valores top de la pila Y / X */
INST(mod)           /* calcula Y % X */
INST(neg)           /* calcula -X */
INST(pwr)           /* calcula Y ^ X */
INST(eval)          /* evalua una variable */
INST(assign)        /* asigna X a una variable */
INST(print)         /* imprime X */
INST(bltin0)        /* llama a una funcion bltin0 (una sin parametros) */
INST(bltin1)        /* llama a una funcion bltin1 (de un parametro) */
INST(bltin2)        /* llama a una funcion bltin2 (de dos parametros) */
INST(ge)            /* operador Y >= X */
INST(le)            /* operador Y <= X */
INST(gt)            /* operador Y > X */
INST(lt)            /* operador Y < X */
INST(eq)            /* operador Y == X */
INST(ne)            /* operador Y != X */
INST(not)           /* operador ! */
INST(and)           /* operador Y && X (sin cortocircuito) */
INST(and_then)      /* operador Y && X (con cortocircuito) */
INST(or)            /* operador Y || X (sin cortocircuito) */
INST(or_else)       /* operador Y || X (con cortocircuito) */
INST(readopcode)    /* lee un valor y lo asigna a una variable */
INST(call)          /* llama a una subrutina con los parametros de la pila */
INST(procret)       /* retorna de un procedimiento definido por el usuario */
INST(funcret)       /* retorna un valor de una funcion definida por el usuario */
INST(argeval)       /* evalua un argumento y lo pone en la pila. */
INST(argassign)     /* asigna el top de la pila a $n.  X -> $n */
INST(prstr)         /* imprime una cadena */
INST(prexpr)        /* imprime una expresion */
INST(symbs)         /* imprime la tabla de simbolos (desaparecera) */
INST(list)          /* lista el codigo del programa */
INST(if_f_goto)     /* salto si el top de la pila es cero */
INST(Goto)          /* salto incondicional */
INST(noop)          /* no operacion, nada */
