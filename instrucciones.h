/* instrucciones.h -- juego de instrucciones de la maquina virtual.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Sat Mar 22 12:23:22 -05 2025
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD
 */

INST(STOP,1)                                   /* para la maquina, termina la ejecucion. */
INST(drop,1)                                   /* elimina un valor de la pila */
INST(dupl,1)                                   /* Duplicar celda */
INST(swap,1)                                   /* Intercambiar celda */
INST(constpush_c,2, SUFF(void, datum_c, prog)) /* introduce un valor constante en la pila */
INST(constpush_d,2, SUFF(void, datum_d, prog))
INST(constpush_f,2, SUFF(void, datum_f, prog))
INST(constpush_i,2, SUFF(void, datum_i, prog))
INST(constpush_l,2, SUFF(void, datum_l, prog))
INST(constpush_s,2, SUFF(void, datum_s, prog))
INST(add_c,1)                                  /* suma los dos valores top de la pila */
INST(add_d,1)
INST(add_f,1)
INST(add_i,1)
INST(add_l,1)
INST(add_s,1)
INST(sub_c,1)                                  /* resta los dos valores top de la pila Y - X */
INST(sub_d,1)
INST(sub_f,1)
INST(sub_i,1)
INST(sub_l,1)
INST(sub_s,1)
INST(mul_c,1)                                  /* multiplica los dos valores top de la pila Y * X */
INST(mul_d,1)
INST(mul_f,1)
INST(mul_i,1)
INST(mul_l,1)
INST(mul_s,1)
INST(divi_c,1)                                 /* divide los dos valores top de la pila Y / X */
INST(divi_d,1)
INST(divi_f,1)
INST(divi_i,1)
INST(divi_l,1)
INST(divi_s,1)
INST(mod_c,1)                                  /* calcula Y % X */
INST(mod_d,1)
INST(mod_f,1)
INST(mod_i,1)
INST(mod_l,1)
INST(mod_s,1)
INST(neg_c,1)                                  /* calcula -X */
INST(neg_d,1)
INST(neg_f,1)
INST(neg_i,1)
INST(neg_l,1)
INST(neg_s,1)
INST(bit_or_c,1)                               /* or de bits */
INST(bit_or_i,1)
INST(bit_or_l,1)
INST(bit_or_s,1)
INST(bit_xor_c,1)                              /* or exclusiva de bits */
INST(bit_xor_i,1)
INST(bit_xor_l,1)
INST(bit_xor_s,1)
INST(bit_and_c,1)                              /* and de bits */
INST(bit_and_i,1)
INST(bit_and_l,1)
INST(bit_and_s,1)
INST(bit_shl_c,1)                              /* despl. bits a la izquierda */
INST(bit_shl_i,1)
INST(bit_shl_l,1)
INST(bit_shl_s,1)
INST(bit_shr_c,1)                              /* despl. bits a la derecha */
INST(bit_shr_i,1)
INST(bit_shr_l,1)
INST(bit_shr_s,1)
INST(bit_not_c,1)                              /* complementa los bits de un entero */
INST(bit_not_i,1)
INST(bit_not_l,1)
INST(bit_not_s,1)
INST(pwr_c,1)                                  /* calcula Y ^^ X */
INST(pwr_d,1)
INST(pwr_f,1)
INST(pwr_i,1)
INST(pwr_l,1)
INST(pwr_s,1)
INST(eval_c,2,      SUFF(void, symb, prog))    /* evalua una variable */
INST(eval_d,2,      SUFF(void, symb, prog))
INST(eval_f,2,      SUFF(void, symb, prog))
INST(eval_i,2,      SUFF(void, symb, prog))
INST(eval_l,2,      SUFF(void, symb, prog))
INST(eval_s,2,      SUFF(void, symb, prog))
INST(assign_c,2,    SUFF(void, symb, prog))    /* asigna X a una variable */
INST(assign_d,2,    SUFF(void, symb, prog))
INST(assign_f,2,    SUFF(void, symb, prog))
INST(assign_i,2,    SUFF(void, symb, prog))
INST(assign_l,2,    SUFF(void, symb, prog))
INST(assign_s,2,    SUFF(void, symb, prog))
INST(argeval_c,2,   SUFF(void, arg_str, prog)) /* evalua un argumento y lo pone en la pila. */
INST(argeval_d,2,   SUFF(void, arg_str, prog))
INST(argeval_f,2,   SUFF(void, arg_str, prog))
INST(argeval_i,2,   SUFF(void, arg_str, prog))
INST(argeval_l,2,   SUFF(void, arg_str, prog))
INST(argeval_s,2,   SUFF(void, arg_str, prog))
INST(argassign_c,2, SUFF(void, arg_str, prog)) /* asigna el top de la pila a $n.  X -> $n */
INST(argassign_d,2, SUFF(void, arg_str, prog))
INST(argassign_f,2, SUFF(void, arg_str, prog))
INST(argassign_i,2, SUFF(void, arg_str, prog))
INST(argassign_l,2, SUFF(void, arg_str, prog))
INST(argassign_s,2, SUFF(void, arg_str, prog))
INST(print_c,1)                                /* imprime X */
INST(print_d,1)
INST(print_f,1)
INST(print_i,1)
INST(print_l,1)
INST(print_s,1)
INST(bltin0,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin0 (una sin parametros) */
INST(bltin1,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin1 (de un parametro) */
INST(bltin2,2,      SUFF(void, symb, prog))    /* llama a una funcion bltin2 (de dos parametros) */
INST(bltin,1,       SUFF(void, arg, prog))     /* llama a una funcion bltin arbitraria */
INST(ge_c,1)                                   /* operador Y >= X */
INST(ge_d,1)
INST(ge_f,1)
INST(ge_i,1)
INST(ge_l,1)
INST(ge_s,1)
INST(le_c,1)                                   /* operador Y <= X */
INST(le_d,1)
INST(le_f,1)
INST(le_i,1)
INST(le_l,1)
INST(le_s,1)
INST(gt_c,1)                                   /* operador Y > X */
INST(gt_d,1)
INST(gt_f,1)
INST(gt_i,1)
INST(gt_l,1)
INST(gt_s,1)
INST(lt_c,1)                                   /* operador Y < X */
INST(lt_d,1)
INST(lt_f,1)
INST(lt_i,1)
INST(lt_l,1)
INST(lt_s,1)
INST(eq_c,1)                                   /* operador Y == X */
INST(eq_d,1)
INST(eq_f,1)
INST(eq_i,1)
INST(eq_l,1)
INST(eq_s,1)
INST(ne_c,1)                                   /* operador Y != X */
INST(ne_d,1)
INST(ne_f,1)
INST(ne_i,1)
INST(ne_l,1)
INST(ne_s,1)
INST(not,1)                                    /* operador ! */
INST(and_then,1,    SUFF(void, addr, prog))    /* operador Y && X (con cortocircuito) */
INST(or_else,1,     SUFF(void, addr, prog))    /* operador Y || X (con cortocircuito) */
INST(call,2,        SUFF(void, symb, prog))    /* llama a una subrutina con los parametros de la pila */
INST(ret,1)                                    /* retorna de un procedimiento definido por el usuario */
INST(prstr,2,       SUFF(void, str, prog))     /* imprime una cadena */
INST(prexpr_c,1)                               /* imprime una expresion */
INST(prexpr_d,1)
INST(prexpr_f,1)
INST(prexpr_i,1)
INST(prexpr_l,1)
INST(prexpr_s,1)
INST(symbs,1)                                  /* imprime la tabla de simbolos (desaparecera) */
INST(symbs_all,2,   SUFF(void, symb, prog))    /* imprime toda la tabla de simbolos */
INST(brkpt,2,       SUFF(void, symb, prog))    /* imprime las variables existentes en el contexto actual */
INST(list,1)                                   /* lista el codigo del programa */
INST(if_f_goto,1,   SUFF(void, addr, prog))    /* salto si el top de la pila es cero */
INST(Goto,1,        SUFF(void, addr, prog))    /* salto incondicional */
INST(noop,1)                                   /* no operacion, nada */
INST(spadd,1,       SUFF(void, arg,  prog))    /* a;ade/substrae del stack pointer */
INST(push_fp, 1)                               /* mete el frame pointer en la pila */
INST(pop_fp, 1)                                /* saca el fp del top de la pila */
INST(move_sp_to_fp, 1)                         /* asigna el fp con el valor del sp. */
INST(c2d,1)                                    /* convertir char hasta double */
INST(c2f,1)                                    /* convertir char hasta float */
INST(c2i,1)                                    /* convertir char hasta int */
INST(c2l,1)                                    /* convertir char hasta long */
INST(c2s,1)                                    /* convertir char hasta short */
INST(d2c,1)                                    /* convertir double hasta char */
INST(d2f,1)                                    /* convertir double hasta float */
INST(d2i,1)                                    /* convertir double hasta int */
INST(d2l,1)                                    /* convertir double hasta long */
INST(d2s,1)                                    /* convertir double hasta short */
INST(f2c,1)                                    /* convertir float hasta char */
INST(f2d,1)                                    /* convertir float hasta double */
INST(f2i,1)                                    /* convertir float hasta int */
INST(f2l,1)                                    /* convertir float hasta long */
INST(f2s,1)                                    /* convertir float hasta short */
INST(i2c,1)                                    /* convertir int hasta char */
INST(i2d,1)                                    /* convertir int hasta double */
INST(i2f,1)                                    /* convertir int hasta float */
INST(i2l,1)                                    /* convertir int hasta long */
INST(i2s,1)                                    /* convertir int hasta short */
INST(l2c,1)                                    /* convertir long hasta char */
INST(l2d,1)                                    /* convertir long hasta double */
INST(l2f,1)                                    /* convertir long hasta float */
INST(l2i,1)                                    /* convertir long hasta int */
INST(l2s,1)                                    /* convertir long hasta short */
INST(s2c,1)                                    /* convertir short hasta char */
INST(s2d,1)                                    /* convertir short hasta double */
INST(s2f,1)                                    /* convertir short hasta float */
INST(s2i,1)                                    /* convertir short hasta int */
INST(s2l,1)                                    /* convertir short hasta long */
