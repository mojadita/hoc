/* binops_eval.h -- evaluation of macros BINOP_EVAL for the
 *                  different functions.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Thu Nov  6 14:30:14 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
BINOP_EVAL(and, _c, chr, chr, &&)
BINOP_EVAL(and, _i, itg, itg, &&)
BINOP_EVAL(and, _l, lng, lng, &&)
BINOP_EVAL(and, _h, sht, sht, &&)

BINOP_EVAL(or, _c, chr, chr, ||)
BINOP_EVAL(or, _i, itg, itg, ||)
BINOP_EVAL(or, _l, lng, lng, ||)
BINOP_EVAL(or, _h, sht, sht, ||)

BINOP_EVAL(minus, _c, chr, chr, -)
BINOP_EVAL(minus, _d, dbl, dbl, -)
BINOP_EVAL(minus, _f, flt, flt, -)
BINOP_EVAL(minus, _i, itg, itg, -)
BINOP_EVAL(minus, _l, lng, lng, -)
BINOP_EVAL(minus, _h, sht, sht, -)

BINOP_EVAL(mult, _c, chr, chr, *)
BINOP_EVAL(mult, _d, dbl, dbl, *)
BINOP_EVAL(mult, _f, flt, flt, *)
BINOP_EVAL(mult, _i, itg, itg, *)
BINOP_EVAL(mult, _l, lng, lng, *)
BINOP_EVAL(mult, _h, sht, sht, *)

BINOP_EVAL(divi, _c, chr, chr, /)
BINOP_EVAL(divi, _d, dbl, dbl, /)
BINOP_EVAL(divi, _f, flt, flt, /)
BINOP_EVAL(divi, _i, itg, itg, /)
BINOP_EVAL(divi, _l, lng, lng, /)
BINOP_EVAL(divi, _h, sht, sht, /)

BINOP_EVAL(bitand, _c, chr, chr, &)
BINOP_EVAL(bitand, _i, itg, itg, &)
BINOP_EVAL(bitand, _l, lng, lng, &)
BINOP_EVAL(bitand, _h, sht, sht, &)

BINOP_EVAL(mod, _c, chr, chr, %)
BINOP_EVAL(mod, _i, itg, itg, %)
BINOP_EVAL(mod, _l, lng, lng, %)
BINOP_EVAL(mod, _h, sht, sht, %)

BINOP_EVAL(bitxor, _c, chr, chr, ^)
BINOP_EVAL(bitxor, _i, itg, itg, ^)
BINOP_EVAL(bitxor, _l, lng, lng, ^)
BINOP_EVAL(bitxor, _h, sht, sht, ^)

BINOP_EVAL(plus, _c, chr, chr, +)
BINOP_EVAL(plus, _d, dbl, dbl, +)
BINOP_EVAL(plus, _f, flt, flt, +)
BINOP_EVAL(plus, _i, itg, itg, +)
BINOP_EVAL(plus, _l, lng, lng, +)
BINOP_EVAL(plus, _h, sht, sht, +)

BINOP_EVAL(lt, _c, itg, chr, <)
BINOP_EVAL(lt, _d, itg, dbl, <)
BINOP_EVAL(lt, _f, itg, flt, <)
BINOP_EVAL(lt, _i, itg, itg, <)
BINOP_EVAL(lt, _l, itg, lng, <)
BINOP_EVAL(lt, _h, itg, sht, <)

BINOP_EVAL(gt, _c, itg, chr, >)
BINOP_EVAL(gt, _d, itg, dbl, >)
BINOP_EVAL(gt, _f, itg, flt, >)
BINOP_EVAL(gt, _i, itg, itg, >)
BINOP_EVAL(gt, _l, itg, lng, >)
BINOP_EVAL(gt, _h, itg, sht, >)

BINOP_EVAL(bitor, _c, chr, chr, |)
BINOP_EVAL(bitor, _i, itg, itg, |)
BINOP_EVAL(bitor, _l, lng, lng, |)
BINOP_EVAL(bitor, _h, sht, sht, |)

BINOP_EVAL(eq, _c, itg, chr, ==)
BINOP_EVAL(eq, _d, itg, dbl, ==)
BINOP_EVAL(eq, _f, itg, flt, ==)
BINOP_EVAL(eq, _i, itg, itg, ==)
BINOP_EVAL(eq, _l, itg, lng, ==)
BINOP_EVAL(eq, _h, itg, sht, ==)

BINOP_EVAL(ge, _c, itg, chr, >=)
BINOP_EVAL(ge, _d, itg, dbl, >=)
BINOP_EVAL(ge, _f, itg, flt, >=)
BINOP_EVAL(ge, _i, itg, itg, >=)
BINOP_EVAL(ge, _l, itg, lng, >=)
BINOP_EVAL(ge, _h, itg, sht, >=)

BINOP_EVAL(le, _c, itg, chr, <=)
BINOP_EVAL(le, _d, itg, dbl, <=)
BINOP_EVAL(le, _f, itg, flt, <=)
BINOP_EVAL(le, _i, itg, itg, <=)
BINOP_EVAL(le, _l, itg, lng, <=)
BINOP_EVAL(le, _h, itg, sht, <=)

BINOP_EVAL(ne, _c, itg, chr, !=)
BINOP_EVAL(ne, _d, itg, dbl, !=)
BINOP_EVAL(ne, _f, itg, flt, !=)
BINOP_EVAL(ne, _i, itg, itg, !=)
BINOP_EVAL(ne, _l, itg, lng, !=)
BINOP_EVAL(ne, _h, itg, sht, !=)
