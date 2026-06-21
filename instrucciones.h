/* instrucciones.h -- juego de instrucciones de la maquina virtual.
 * Author: Luis Colorado <luiscoloradourcola@gmail.com>
 *       & Edward Rivas <rivastkw@gmail.com>
 * Date: Sat Mar 22 12:23:22 -05 2025
 * Copyright: (c) 2025-2026 Luis Colorado.  All rights reserved.
 * License: BSD
 */

INST(STOP,1)                                   /* stops the virtual machine. */
INST(drop,1)                                   /* drops a value from the stack */
INST(dupl,1)                                   /* duplicate top value of stack */
INST(swap,1)                                   /* swaps top and the value behind in the top of the stack */
INST(constpush_c,2, SUFF(void, datum_c, prog)) /* enters a constant in the stack */
INST(constpush_d,2, SUFF(void, datum_d, prog))
INST(constpush_f,2, SUFF(void, datum_f, prog))
INST(constpush_i,2, SUFF(void, datum_i, prog))
INST(constpush_l,2, SUFF(void, datum_l, prog))
INST(constpush_s,2, SUFF(void, datum_s, prog))
INST(add_c,1)                                  /* adds the top two values of the stack */
INST(add_d,1)
INST(add_f,1)
INST(add_i,1)
INST(add_l,1)
INST(add_s,1)
INST(sub_c,1)                                  /* subtracts the top two values of stack */
INST(sub_d,1)
INST(sub_f,1)
INST(sub_i,1)
INST(sub_l,1)
INST(sub_s,1)
INST(mul_c,1)                                  /* multiplies the top two values of the stack */
INST(mul_d,1)
INST(mul_f,1)
INST(mul_i,1)
INST(mul_l,1)
INST(mul_s,1)
INST(divi_c,1)                                 /* divides the top two values of the stack */
INST(divi_d,1)
INST(divi_f,1)
INST(divi_i,1)
INST(divi_l,1)
INST(divi_s,1)
INST(mod_c,1)                                  /*  calculates the remainder of the division */
INST(mod_d,1)
INST(mod_f,1)
INST(mod_i,1)
INST(mod_l,1)
INST(mod_s,1)
INST(neg_c,1)                                  /* negates the top value of the stack */
INST(neg_d,1)
INST(neg_f,1)
INST(neg_i,1)
INST(neg_l,1)
INST(neg_s,1)
INST(bit_or_c,1)                               /* bit or */
INST(bit_or_i,1)
INST(bit_or_l,1)
INST(bit_or_s,1)
INST(bit_xor_c,1)                              /* bit xor */
INST(bit_xor_i,1)
INST(bit_xor_l,1)
INST(bit_xor_s,1)
INST(bit_and_c,1)                              /* bit and */
INST(bit_and_i,1)
INST(bit_and_l,1)
INST(bit_and_s,1)
INST(bit_shl_c,1)                              /* shifts bits left */
INST(bit_shl_i,1)
INST(bit_shl_l,1)
INST(bit_shl_s,1)
INST(bit_shr_c,1)                              /* shifts bits right (arithmetic) */
INST(bit_shr_i,1)
INST(bit_shr_l,1)
INST(bit_shr_s,1)
INST(bit_not_c,1)                              /* complements bits on an integer */
INST(bit_not_i,1)
INST(bit_not_l,1)
INST(bit_not_s,1)
INST(pwr_c,1)                                  /* calculates Y ^^ X --exponentiation operator-- */
INST(pwr_d,1)
INST(pwr_f,1)
INST(pwr_i,1)
INST(pwr_l,1)
INST(pwr_s,1)
INST(eval_c,2,      SUFF(void, symb, prog))    /* evaluates a variable (global) */
INST(eval_d,2,      SUFF(void, symb, prog))
INST(eval_f,2,      SUFF(void, symb, prog))
INST(eval_i,2,      SUFF(void, symb, prog))
INST(eval_l,2,      SUFF(void, symb, prog))
INST(eval_s,2,      SUFF(void, symb, prog))
INST(assign_c,2,    SUFF(void, symb, prog))    /* assigns top value to a variable */
INST(assign_d,2,    SUFF(void, symb, prog))
INST(assign_f,2,    SUFF(void, symb, prog))
INST(assign_i,2,    SUFF(void, symb, prog))
INST(assign_l,2,    SUFF(void, symb, prog))
INST(assign_s,2,    SUFF(void, symb, prog))
INST(argeval_c,2,   SUFF(void, arg_str, prog)) /* evaluates an argument/local variable to the stack. */
INST(argeval_d,2,   SUFF(void, arg_str, prog))
INST(argeval_f,2,   SUFF(void, arg_str, prog))
INST(argeval_i,2,   SUFF(void, arg_str, prog))
INST(argeval_l,2,   SUFF(void, arg_str, prog))
INST(argeval_s,2,   SUFF(void, arg_str, prog))
INST(argassign_c,2, SUFF(void, arg_str, prog)) /* assigns the top value to an argument/local variable */
INST(argassign_d,2, SUFF(void, arg_str, prog))
INST(argassign_f,2, SUFF(void, arg_str, prog))
INST(argassign_i,2, SUFF(void, arg_str, prog))
INST(argassign_l,2, SUFF(void, arg_str, prog))
INST(argassign_s,2, SUFF(void, arg_str, prog))
INST(print_c,1)                                /* prints top value */
INST(print_d,1)
INST(print_f,1)
INST(print_i,1)
INST(print_l,1)
INST(print_s,1)
INST(bltin,1,       SUFF(void, arg, prog))     /* calls an arbitrary builtin */
INST(ge_c,1)                                   /* >= operator */
INST(ge_d,1)
INST(ge_f,1)
INST(ge_i,1)
INST(ge_l,1)
INST(ge_s,1)
INST(le_c,1)                                   /* <= operator */
INST(le_d,1)
INST(le_f,1)
INST(le_i,1)
INST(le_l,1)
INST(le_s,1)
INST(gt_c,1)                                   /* > operator */
INST(gt_d,1)
INST(gt_f,1)
INST(gt_i,1)
INST(gt_l,1)
INST(gt_s,1)
INST(lt_c,1)                                   /* < operator */
INST(lt_d,1)
INST(lt_f,1)
INST(lt_i,1)
INST(lt_l,1)
INST(lt_s,1)
INST(eq_c,1)                                   /* == operator */
INST(eq_d,1)
INST(eq_f,1)
INST(eq_i,1)
INST(eq_l,1)
INST(eq_s,1)
INST(ne_c,1)                                   /* != operator */
INST(ne_d,1)
INST(ne_f,1)
INST(ne_i,1)
INST(ne_l,1)
INST(ne_s,1)
INST(not,1)                                    /* ! operator */
INST(and_then,1,    SUFF(void, addr, prog))    /* && operator (shortcircuited) */
INST(or_else,1,     SUFF(void, addr, prog))    /* || operator (shortcircuited) */
INST(call,2,        SUFF(void, symb, prog))    /* calls a suboutine passing parameters on stack */
INST(ret,1)                                    /* returns from a subroutine */
INST(prstr,2,       SUFF(void, str, prog))     /* prints a string literal */
INST(prexpr_c,1)                               /* prints an expression */
INST(prexpr_d,1)
INST(prexpr_f,1)
INST(prexpr_i,1)
INST(prexpr_l,1)
INST(prexpr_s,1)
INST(symbs,1)                                  /* prints the symbol table (will dissapear) */
INST(symbs_all,2,   SUFF(void, symb, prog))    /* prints the whole symbol table at code position */
INST(brkpt,2,       SUFF(void, symb, prog))    /* prints all variables in current context*/
INST(list,1)                                   /* lists the assembled code */
INST(if_f_goto,1,   SUFF(void, addr, prog))    /* jumps if the stack top is zero */
INST(Goto,1,        SUFF(void, addr, prog))    /* unconditional jump */
INST(noop,1)                                   /* no oparation */
INST(spadd,1,       SUFF(void, arg,  prog))    /* adds/subtract to the stack pointer */
INST(push_fp, 1)                               /* pushes the frame pointer on top of stack */
INST(pop_fp, 1)                                /* pulls the frame pointer from the top of stack */
INST(move_sp_to_fp, 1)                         /* moves sp to fp. */
INST(c2d,1)                                    /* converts char to double */
INST(c2f,1)                                    /* converts char to float */
INST(c2i,1)                                    /* converts char to int */
INST(c2l,1)                                    /* converts char to long */
INST(c2s,1)                                    /* converts char to short */
INST(d2c,1)                                    /* converts double to char */
INST(d2f,1)                                    /* converts double to float */
INST(d2i,1)                                    /* converts double to int */
INST(d2l,1)                                    /* converts double to long */
INST(d2s,1)                                    /* converts double to short */
INST(f2c,1)                                    /* converts float to char */
INST(f2d,1)                                    /* converts float to double */
INST(f2i,1)                                    /* converts float to int */
INST(f2l,1)                                    /* converts float to long */
INST(f2s,1)                                    /* converts float to short */
INST(i2c,1)                                    /* converts int to char */
INST(i2d,1)                                    /* converts int to double */
INST(i2f,1)                                    /* converts int to float */
INST(i2l,1)                                    /* converts int to long */
INST(i2s,1)                                    /* converts int to short */
INST(l2c,1)                                    /* converts long to char */
INST(l2d,1)                                    /* converts long to double */
INST(l2f,1)                                    /* converts long to float */
INST(l2i,1)                                    /* converts long to int */
INST(l2s,1)                                    /* converts long to short */
INST(s2c,1)                                    /* converts short to char */
INST(s2d,1)                                    /* converts short to double */
INST(s2f,1)                                    /* converts short to float */
INST(s2i,1)                                    /* converts short to int */
INST(s2l,1)                                    /* converts short to long */
