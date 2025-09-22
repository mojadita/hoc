/* instrucciones.h -- juego de instrucciones de la maquina virtual.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Mar 22 12:23:22 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

INST(STOP,1)                                   /* para la maquina, termina la ejecucion. */
INST(drop,1)                                   /* elimina un valor de la pila */
INST(constpush,2,   SUFF(void, datum, prog))   /* introduce un valor constante en la pila */
INST(constpush_d,2, SUFF(void, datum_d, prog))
INST(constpush_l,2, SUFF(void, datum_l, prog))
INST(add,1)                                    /* suma los dos valores top de la pila */
INST(add_d,1)
INST(add_l,1)
INST(sub,1)                                    /* resta los dos valores top de la pila Y - X */
INST(sub_d,1)
INST(sub_l,1)
INST(mul,1)                                    /* multiplica los dos valores top de la pila Y * X */
INST(mul_d,1)
INST(mul_l,1)
INST(divi,1)                                   /* divide los dos valores top de la pila Y / X */
INST(divi_d,1)
INST(divi_l,1)
INST(mod,1)                                    /* calcula Y % X */
INST(mod_d,1)
INST(mod_l,1)
INST(neg,1)                                    /* calcula -X */
INST(neg_d,1)
INST(neg_l,1)
INST(pwr,1)                                    /* calcula Y ^^ X */
INST(pwr_d,1)
INST(pwr_l,1)
INST(eval,2,        SUFF(void, symb, prog))    /* evalua una variable */
INST(eval_c,2,      SUFF(void, symb, prog))    /* evalua una variable */
INST(eval_d,2,      SUFF(void, symb, prog))
INST(eval_f,2,      SUFF(void, symb, prog))
INST(eval_i,2,      SUFF(void, symb, prog))
INST(eval_l,2,      SUFF(void, symb, prog))
INST(eval_s,2,      SUFF(void, symb, prog))
INST(assign,2,      SUFF(void, symb, prog))    /* asigna X a una variable */
INST(assign_c,2,    SUFF(void, symb, prog))
INST(assign_d,2,    SUFF(void, symb, prog))
INST(assign_f,2,    SUFF(void, symb, prog))
INST(assign_i,2,    SUFF(void, symb, prog))
INST(assign_l,2,    SUFF(void, symb, prog))
INST(assign_s,2,    SUFF(void, symb, prog))
INST(print,1)                                  /* imprime X */
INST(bltin0,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin0 (una sin parametros) */
INST(bltin1,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin1 (de un parametro) */
INST(bltin2,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin2 (de dos parametros) */
INST(ge,1)                                     /* operador Y >= X */
INST(ge_d,1)                                   /* operador Y >= X */
INST(ge_l,1)
INST(le,1)                                     /* operador Y <= X */
INST(le_d,1)                                   /* operador Y <= X */
INST(le_l,1)
INST(gt,1)                                     /* operador Y > X */
INST(gt_d,1)                                   /* operador Y > X */
INST(gt_l,1)
INST(lt,1)                                     /* operador Y < X */
INST(lt_d,1)                                   /* operador Y < X */
INST(lt_l,1)
INST(eq,1)                                     /* operador Y == X */
INST(eq_d,1)                                   /* operador Y == X */
INST(eq_l,1)
INST(ne,1)                                     /* operador Y != X */
INST(ne_d,1)                                   /* operador Y != X */
INST(ne_l,1)
INST(not,1)                                    /* operador ! */
INST(and_then,1,    SUFF(void, addr, prog))    /* operador Y && X (con cortocircuito) */
INST(or_else,1,     SUFF(void, addr, prog))    /* operador Y || X (con cortocircuito) */
INST(call,2,        SUFF(void, symb, prog))    /* llama a una subrutina con los parametros de la pila */
INST(ret,1)                                    /* retorna de un procedimiento definido por el usuario */
INST(argeval,2,     SUFF(void, arg_str, prog)) /* evalua un argumento y lo pone en la pila. */
INST(argeval_c,2,   SUFF(void, arg_str, prog)) /* evalua un argumento y lo pone en la pila. */
INST(argeval_d,2,   SUFF(void, arg_str, prog))
INST(argeval_f,2,   SUFF(void, arg_str, prog))
INST(argeval_i,2,   SUFF(void, arg_str, prog))
INST(argeval_l,2,   SUFF(void, arg_str, prog))
INST(argassign,2,   SUFF(void, arg_str, prog)) /* asigna el top de la pila a $n.  X -> $n */
INST(argassign_c,2, SUFF(void, arg_str, prog)) /* asigna el top de la pila a $n.  X -> $n */
INST(argassign_d,2, SUFF(void, arg_str, prog))
INST(argassign_f,2, SUFF(void, arg_str, prog))
INST(argassign_i,2, SUFF(void, arg_str, prog))
INST(argassign_l,2, SUFF(void, arg_str, prog))
INST(prstr,2,       SUFF(void, str, prog))     /* imprime una cadena */
INST(prexpr,1)                                 /* imprime una expresion */
INST(prexpr_i,1)                               /* imprime una expresion */
INST(prexpr_d,1)
INST(symbs,1)                                  /* imprime la tabla de simbolos (desaparecera) */
INST(symbs_all,2,   SUFF(void, symb, prog))    /* imprime toda la tabla de simbolos */
INST(brkpt,2,       SUFF(void, symb, prog))    /* imprime las variables existentes en el contexto actual */
INST(list,1)                                   /* lista el codigo del programa */
INST(if_f_goto,1,   SUFF(void, addr, prog))    /* salto si el top de la pila es cero */
INST(Goto,1,        SUFF(void, addr, prog))    /* salto incondicional */
INST(noop,1)                                   /* no operacion, nada */
INST(inceval,2,     SUFF(void, symb, prog))    /* incremento de variable+eval */
INST(inceval_c,2,   SUFF(void, symb, prog))    /* incremento de variable+eval */
INST(inceval_d,2,   SUFF(void, symb, prog))
INST(inceval_f,2,   SUFF(void, symb, prog))
INST(inceval_i,2,   SUFF(void, symb, prog))
INST(inceval_l,2,   SUFF(void, symb, prog))
INST(evalinc,2,     SUFF(void, symb, prog))    /* eval+incremento de variable */
INST(evalinc_c,2,   SUFF(void, symb, prog))    /* eval+incremento de variable */
INST(evalinc_d,2,   SUFF(void, symb, prog))
INST(evalinc_f,2,   SUFF(void, symb, prog))
INST(evalinc_i,2,   SUFF(void, symb, prog))
INST(evalinc_l,2,   SUFF(void, symb, prog))
INST(deceval,2,     SUFF(void, symb, prog))    /* decremento+eval de variable */
INST(deceval_c,2,   SUFF(void, symb, prog))    /* decremento+eval de variable */
INST(deceval_d,2,   SUFF(void, symb, prog))
INST(deceval_f,2,   SUFF(void, symb, prog))
INST(deceval_i,2,   SUFF(void, symb, prog))
INST(deceval_l,2,   SUFF(void, symb, prog))
INST(evaldec,2,     SUFF(void, symb, prog))    /* eval+decremento de variable */
INST(evaldec_c,2,   SUFF(void, symb, prog))    /* eval+decremento de variable */
INST(evaldec_d,2,   SUFF(void, symb, prog))
INST(evaldec_f,2,   SUFF(void, symb, prog))
INST(evaldec_i,2,   SUFF(void, symb, prog))
INST(evaldec_l,2,   SUFF(void, symb, prog))
INST(addvar,2,      SUFF(void, symb, prog))    /* añade de la pila a variable */
INST(addvar_c,2,    SUFF(void, symb, prog))    /* añade de la pila a variable */
INST(addvar_d,2,    SUFF(void, symb, prog))
INST(addvar_f,2,    SUFF(void, symb, prog))
INST(addvar_i,2,    SUFF(void, symb, prog))
INST(addvar_l,2,    SUFF(void, symb, prog))
INST(addvar_s,2,    SUFF(void, symb, prog))
INST(subvar,2,      SUFF(void, symb, prog))    /* substrae de la pila a variable */
INST(subvar_c,2,    SUFF(void, symb, prog))    /* substrae de la pila a variable */
INST(subvar_d,2,    SUFF(void, symb, prog))
INST(subvar_f,2,    SUFF(void, symb, prog))
INST(subvar_i,2,    SUFF(void, symb, prog))
INST(subvar_l,2,    SUFF(void, symb, prog))
INST(subvar_s,2,    SUFF(void, symb, prog))
INST(mulvar,2,      SUFF(void, symb, prog))    /* multiplica de la pila a variable */
INST(mulvar_c,2,    SUFF(void, symb, prog))    /* multiplica de la pila a variable */
INST(mulvar_d,2,    SUFF(void, symb, prog))
INST(mulvar_f,2,    SUFF(void, symb, prog))
INST(mulvar_i,2,    SUFF(void, symb, prog))
INST(mulvar_l,2,    SUFF(void, symb, prog))
INST(mulvar_s,2,    SUFF(void, symb, prog))
INST(divvar,2,      SUFF(void, symb, prog))    /* divide de la pila a variable */
INST(divvar_c,2,    SUFF(void, symb, prog))    /* divide de la pila a variable */
INST(divvar_d,2,    SUFF(void, symb, prog))
INST(divvar_f,2,    SUFF(void, symb, prog))
INST(divvar_i,2,    SUFF(void, symb, prog))
INST(divvar_l,2,    SUFF(void, symb, prog))
INST(divvar_s,2,    SUFF(void, symb, prog))
INST(modvar,2,      SUFF(void, symb, prog))    /* modulo de la pila a variable */
INST(modvar_c,2,    SUFF(void, symb, prog))    /* modulo de la pila a variable */
INST(modvar_d,2,    SUFF(void, symb, prog))
INST(modvar_f,2,    SUFF(void, symb, prog))
INST(modvar_i,2,    SUFF(void, symb, prog))
INST(modvar_l,2,    SUFF(void, symb, prog))
INST(modvar_s,2,    SUFF(void, symb, prog))
INST(pwrvar,2,      SUFF(void, symb, prog))    /* potencia de la pila a variable */
INST(pwrvar_c,2,    SUFF(void, symb, prog))    /* potencia de la pila a variable */
INST(pwrvar_d,2,    SUFF(void, symb, prog))
INST(pwrvar_f,2,    SUFF(void, symb, prog))
INST(pwrvar_i,2,    SUFF(void, symb, prog))
INST(pwrvar_l,2,    SUFF(void, symb, prog))
INST(pwrvar_s,2,    SUFF(void, symb, prog))
INST(arginc,2,      SUFF(void, arg_str,  prog))/* postincremento de argumento */
INST(arginc_c,2,    SUFF(void, arg_str,  prog))/* postincremento de argumento */
INST(arginc_d,2,    SUFF(void, arg_str,  prog))
INST(arginc_f,2,    SUFF(void, arg_str,  prog))
INST(arginc_i,2,    SUFF(void, arg_str,  prog))
INST(arginc_l,2,    SUFF(void, arg_str,  prog))
INST(arginc_s,2,    SUFF(void, arg_str,  prog))
INST(incarg,2,      SUFF(void, arg_str,  prog))/* preincremento de argumento */
INST(incarg_c,2,    SUFF(void, arg_str,  prog))/* preincremento de argumento */
INST(incarg_d,2,    SUFF(void, arg_str,  prog))
INST(incarg_f,2,    SUFF(void, arg_str,  prog))
INST(incarg_i,2,    SUFF(void, arg_str,  prog))
INST(incarg_l,2,    SUFF(void, arg_str,  prog))
INST(incarg_s,2,    SUFF(void, arg_str,  prog))
INST(decarg,2,      SUFF(void, arg_str,  prog))/* predecremento de argumento */
INST(decarg_c,2,    SUFF(void, arg_str,  prog))/* predecremento de argumento */
INST(decarg_d,2,    SUFF(void, arg_str,  prog))
INST(decarg_f,2,    SUFF(void, arg_str,  prog))
INST(decarg_i,2,    SUFF(void, arg_str,  prog))
INST(decarg_l,2,    SUFF(void, arg_str,  prog))
INST(decarg_s,2,    SUFF(void, arg_str,  prog))
INST(argdec,2,      SUFF(void, arg_str,  prog))
INST(argdec_c,2,    SUFF(void, arg_str,  prog))/* postdecremento de argumento */
INST(argdec_d,2,    SUFF(void, arg_str,  prog))
INST(argdec_f,2,    SUFF(void, arg_str,  prog))
INST(argdec_i,2,    SUFF(void, arg_str,  prog))
INST(argdec_l,2,    SUFF(void, arg_str,  prog))
INST(argdec_s,2,    SUFF(void, arg_str,  prog))
INST(addarg,2,      SUFF(void, arg_str,  prog))
INST(addarg_c,2,    SUFF(void, arg_str,  prog))/* añade de la pila a argumento */
INST(addarg_d,2,    SUFF(void, arg_str,  prog))
INST(addarg_f,2,    SUFF(void, arg_str,  prog))
INST(addarg_i,2,    SUFF(void, arg_str,  prog))
INST(addarg_l,2,    SUFF(void, arg_str,  prog))
INST(addarg_s,2,    SUFF(void, arg_str,  prog))
INST(subarg,2,      SUFF(void, arg_str,  prog))/* substrae de la pila a argumento */
INST(subarg_c,2,    SUFF(void, arg_str,  prog))/* substrae de la pila a argumento */
INST(subarg_d,2,    SUFF(void, arg_str,  prog))
INST(subarg_f,2,    SUFF(void, arg_str,  prog))
INST(subarg_i,2,    SUFF(void, arg_str,  prog))
INST(subarg_l,2,    SUFF(void, arg_str,  prog))
INST(subarg_s,2,    SUFF(void, arg_str,  prog))
INST(mularg,2,      SUFF(void, arg_str,  prog))/* multiplica de la pila a argumento */
INST(mularg_c,2,    SUFF(void, arg_str,  prog))/* multiplica de la pila a argumento */
INST(mularg_d,2,    SUFF(void, arg_str,  prog))
INST(mularg_f,2,    SUFF(void, arg_str,  prog))
INST(mularg_i,2,    SUFF(void, arg_str,  prog))
INST(mularg_l,2,    SUFF(void, arg_str,  prog))
INST(mularg_s,2,    SUFF(void, arg_str,  prog))
INST(divarg,2,      SUFF(void, arg_str,  prog))/* divide de la pila a argumento */
INST(divarg_c,2,    SUFF(void, arg_str,  prog))/* divide de la pila a argumento */
INST(divarg_d,2,    SUFF(void, arg_str,  prog))
INST(divarg_f,2,    SUFF(void, arg_str,  prog))
INST(divarg_i,2,    SUFF(void, arg_str,  prog))
INST(divarg_l,2,    SUFF(void, arg_str,  prog))
INST(divarg_s,2,    SUFF(void, arg_str,  prog))
INST(modarg,2,      SUFF(void, arg_str,  prog))/* modulo de la pila a argumento */
INST(modarg_c,2,    SUFF(void, arg_str,  prog))/* modulo de la pila a argumento */
INST(modarg_d,2,    SUFF(void, arg_str,  prog))
INST(modarg_f,2,    SUFF(void, arg_str,  prog))
INST(modarg_i,2,    SUFF(void, arg_str,  prog))
INST(modarg_l,2,    SUFF(void, arg_str,  prog))
INST(modarg_s,2,    SUFF(void, arg_str,  prog))
INST(pwrarg,2,      SUFF(void, arg_str,  prog))/* potencia de la pila a argumento */
INST(pwrarg_c,2,    SUFF(void, arg_str,  prog))/* potencia de la pila a argumento */
INST(pwrarg_d,2,    SUFF(void, arg_str,  prog))
INST(pwrarg_f,2,    SUFF(void, arg_str,  prog))
INST(pwrarg_i,2,    SUFF(void, arg_str,  prog))
INST(pwrarg_l,2,    SUFF(void, arg_str,  prog))
INST(pwrarg_s,2,    SUFF(void, arg_str,  prog))
INST(spadd,1,       SUFF(void, arg,  prog))/* a;ade/substrae del stack pointer */
INST(push_fp, 1)                         /* mete el frame pointer en la pila */
INST(pop_fp, 1)                          /* saca el fp del top de la pila */
INST(move_sp_to_fp, 1)                   /* asigna el fp con el valor del sp. */
INST(d2f,1)                              /* convertir double hasta float */
INST(d2i,1)                              /* convertir double hasta int */
INST(d2l,1)                              /* convertir double hasta long */
INST(f2d,1)                              /* convertir float hasta double */
INST(f2i,1)                              /* convertir float hasta int */
INST(f2l,1)                              /* convertir float hasta long */
INST(i2d,1)                              /* convertir int hasta double */
INST(i2f,1)                              /* convertir int hasta float */
INST(i2l,1)                              /* convertir int hasta long */
INST(l2d,1)                              /* convertir long hasta double */
INST(l2f,1)                              /* convertir long hasta float */
INST(l2i,1)                              /* convertir long hasta int */
