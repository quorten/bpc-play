/* Math subroutines that cross between ABNumx1 and ABNumx2 built-in C
   integer data types, designed to be similar to GMP (GNU Multiple
   Precision) subroutine calling convention.  If this is a ".s.h"
   file, then it is generalized to all C integer data types.  */

#ifndef AM_C_ABNumx1_ABNumx2
#define AM_C_ABNumx1_ABNumx2

/*

Template parameters:

* ABNumx1: Scalar number type, base width.

* ABUNumx1: Unsigned scalar number type, base width.

* ABNumx2: Scalar number type, double width, to be used as the result
  of multiplications.  Basically only special for fixed-width integer
  arithmetic.  For floating point and bignum data types, this is the
  same as ABNumx1.

* ABUNumx2: Unsigned scalar number type, double width.

* ABBitIdx: Integer data type for referencing bit indexes up to the
  width of ABNumx2, i.e. unsigned char.  Must be a built-in C integer
  data type.  Used in base 2 logarithm (shifting) calculations.

* brnumx1: Abbreviated symbol for abstract number, base width, for use
  in higher level type names, functions, etc.

* brunumx1: Abbreviated symbol for unsigned abstract number, base
  width, for use in higher level type names, functions, etc.

* brnumx2: Abbreviated symbol for abstract number, double-width, for
  use in higher level type names, functions, etc.

* brunumx2: Abbreviated symbol for unsigned abstract number,
  double-width, for use in higher level type names, functions, etc.

* brbitidx: Abbreviated symbol for bit index integer data type, for
  use in higher level type names, functions, etc.

* v_ABNumx2_X1_THRESHOLD: Comparison threshold to check of an ABNumx2
  is too large to fit inside an ABNumx1.  For example, for
  0x100000000LL for 64-bit versus 32-bit.

* v_ABNumx2_X1_THRESHOLD_LOG2: Base 2 logarithm of
  v_ABNumx2_X1_THRESHOLD.

* v_ABNumx1_NO_X2: Define this as 1 if the double-width scalar number
  type is the same as the single-width one, i.e. in the case of using
  floating point or arbitrary precision arithmetic.  Otherwise, define
  it as zero.

*/

#define ABNumx2_X1_THRESHOLD v_ABNumx2_X1_THRESHOLD
#define ABNumx2_X1_THRESHOLD_LOG2 v_ABNumx2_X1_THRESHOLD_LOG2
#define ABNumx1_NO_X2 v_ABNumx1_NO_X2

/* For machine integer (i.e. C built-in) data types, we define both
   "two-parameter" and "three-parameter" (or equivalent) versions of
   the subroutines.  The three parameter versions must be used in all
   code that must generalize to bignums, but the two-parameter
   versions can be used to greater convenience in code where such
   generalization is not necessary.

   Sign test and comparison operations used for branching decisions by
   definition don't need "three-parameter" versions since the result
   will always fit in a machine integer.

   PLEASE NOTE: For proper generalization to bignums, every time there
   is a destination operand, it must have already been "initialized"
   to guarantee it has the proper object format.  Of course, with C
   built-in data types, initialization is a no-op.  */

/* For convenience, include the basic subroutines for single-width and
   double-width.  */
#include "amcint_brnumx1.h"
#include "amcint_brnumx2.h"

/* Convert ABNumx1 to ABNumx2.  */
#define ab_cvt1_brnumx2_brnumx1(b) ((ABNumx2)(b))
#define ab_cvt2_brnumx2_brnumx1(a, b) ((a) = (ABNumx2)(b))
/* Convert ABNumx2 to ABNumx1.  */
#define ab_cvt1_brnumx1_brnumx2(b) ((ABNumx1)(b))
#define ab_cvt2_brnumx1_brnumx2(a, b) ((a) = (ABNumx1)(b))

/* Multiply two ABNumx1 numbers and return the result as an
   ABNumx2.  */
#define ab_mul2_brnumx2_brnumx1(b, c) ((ABNumx2)(b) * (c))
#define ab_mul3_brnumx2_brnumx1(a, b, c) ((a) = (ABNumx2)(b) * (c))

/* Divide a double-width number by a single-width number, this can be
   more efficient than converting the divisor to double-width.  Result
   is double-width.  */
#define ab_div2_brnumx2_brnumx2_brnumx1(b, c) ((b) / (c))
#define ab_div3_brnumx2_brnumx2_brnumx1(a, b, c) ((a) = (b) / (c))

/* Divide a double-width number by a single-width number, this can be
   more efficient than converting the divisor to double-width.  Result
   is single-width.  */
#define ab_div2_brnumx1_brnumx2_brnumx1(b, c) ((ABNumx1)((b) / (c)))
#define ab_div3_brnumx1_brnumx2_brnumx1(a, b, c) \
  ((a) = (ABNumx1)((b) / (c)))

/* Multiply-divide using double-width intermediates.  */
#define ab_muldiv3_brnumx1(b, c, d) \
  ((ABNumx1)((ABNumx2)(b) * (c) / (d)))
#define ab_muldiv4_brnumx1(a, b, c, d) \
  ((a) = (ABNumx1)((ABNumx2)(b) * (c) / (d)))

/* Shift left, divide using double-width intermediates.  */
#define ab_shldiv3_brnumx1(b, c, d) \
  ((ABNumx1)(((ABNumx2)(b) << (c)) / (d)))
#define ab_shldiv4_brnumx1(a, b, c, d) \
  ((a) = (ABNumx1)(((ABNumx2)(b) << (c)) / (d)))

/* Multiply, shift right using double-width intermediates, with
   symmetric positive/negative shift behavior.  `q` must not be
   zero.  */
#define ab_mulshr3_brnumx1(b, c, q) \
  ab_fsyshr2_brnumx2((ABNumx2)(b) * (c), q)
#define ab_mulshr4_brnumx1(a, b, c, q) \
  ((a) = ab_fsyshr2_brnumx2((ABNumx2)(b) * (c), q))

#endif /* not AM_C_ABNumx1_ABNumx2 */
