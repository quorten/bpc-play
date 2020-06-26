/* Math subroutines for ABNumx1 built-in C integer data types,
   designed to be similar to GMP (GNU Multiple Precision) subroutine
   calling convention.  If this is a ".s.h" file, then it is
   generalized to all C integer data types.  */

#ifndef AM_C_ABNumx1
#define AM_C_ABNumx1

/*

Template parameters:

* ABNumx1: Scalar number type, base width.

* ABUNumx1: Unsigned scalar number type, base width.

* ABBitIdx: Integer data type for referencing bit indexes up to the
  width of ABNumx2, i.e. unsigned char.  Must be a built-in C integer
  data type.  Used in base 2 logarithm (shifting) calculations.

* brnumx1: Abbreviated symbol for abstract number, base width, for use
  in higher level type names, functions, etc.

* brunumx1: Abbreviated symbol for unsigned abstract number, base
  width, for use in higher level type names, functions, etc.

* brbitidx: Abbreviated symbol for bit index integer data type, for
  use in higher level type names, functions, etc.

*/

/* Essentially a GMP (GNU Multiple Precision) API facsimile that is
   designed to be able to wrap fixed-width integers in the event you
   don't need to use GMP.  And in that event, the operators are
   templated to your specific integer data types.  There are a few
   extensions specifically for optimal routines, i.e. shifting, with
   binary integer arithmetic.  Pretty nice, eh?  */

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

/* Initialize storage, numeric value is undefined.  */
#define ab_init1_brnumx1(a) ()
/* Initialize to zero.  */
#define ab_initz1_brnumx1(a) ((a) = 0)

/* Destroy storage once it is no longer needed.  */
#define ab_clear1_brnumx1(a) ()

/* Copy the value of an ABNumx1.  */
#define ab_set2_brnumx1(a, b) ((a) = (b))

/* Set the value of an ABNumx1 to zero.  */
#define ab_setz1_brnumx1(a) ((a) = 0)

/* Convert ABNumx1 to ABUNumx1.  */
#define ab_cvt1_brunumx1_brnumx1(b) ((ABUNumx1)(b))
#define ab_cvt2_brunumx1_brnumx1(a, b) ((a) = (ABUNumx1)(b))
/* Convert ABUNumx1 to ABNumx1.  */
#define ab_cvt1_brnumx1_brunumx1(b) ((ABNumx1)(b))
#define ab_cvt2_brnumx1_brunumx1(a, b) ((a) = (ABNumx1)(b))

/* Shift left.  Mathematically, (a * 2^b) for unsigned ONLY.  */
#define ab_shl2_brnumx1(b, c) ((b) << (c))
#define ab_shl3_brnumx1(a, b, c) ((a) = (b) << (c))
/* Shift right.  Mathematically, (a / 2^b) for unsigned ONLY.  */
#define ab_shr2_brnumx1(b, c) ((b) >> (c))
#define ab_shr3_brnumx1(a, b, c) ((a) = (b) >> (c))

#define ab_neg1_brnumx1(b) (-(b))
#define ab_neg2_brnumx1(a, b) ((a) = -(b))
#define ab_inc1_brnumx1(b) ((b) + 1)
#define ab_inc2_brnumx1(a, b) ((a) = (b) + 1)
#define ab_dec1_brnumx1(b) ((b) - 1)
#define ab_dec2_brnumx1(a, b) ((a) = (b) - 1)
#define ab_add2_brnumx1(b, c) ((b) + (c))
#define ab_add3_brnumx1(a, b, c) ((a) = (b) + (c))
#define ab_sub2_brnumx1(b, c) ((b) - (c))
#define ab_sub3_brnumx1(a, b, c) ((a) = (b) - (c))
#define ab_mul2_brnumx1(b, c) ((b) * (c))
#define ab_mul3_brnumx1(a, b, c) ((a) = (b) * (c))
#define ab_div2_brnumx1(b, c) ((b) / (c))
#define ab_div3_brnumx1(a, b, c) ((a) = (b) / (c))
#define ab_mod2_brnumx1(b, c) ((b) % (c))
#define ab_mod3_brnumx1(a, b, c) ((a) = (b) % (c))

/* Test and return the sign (+n, -n, or zero) of a number.  The
   magnitude of the non-zero results is arbitrary but it will fit in a
   C built-in data type.  */
#define ab_sgn1_brnumx1(b) (b)
/* Compare two numbers by subtraction (a - b), and return the sign
   (+1, -1, or zero) of the result.  */
#define ab_cmp2_brnumx1(b, c) ((b) - (c))

/********************************************************************/
/* Convenience routines */

/* Compute `2^b` by shifting, `b` must be unsigned.  */
#define ab_powtwo1_brnumx1(b) ((ABNumx1)1 << (b))
#define ab_powtwo2_brnumx1(a, b) ((a) = (ABNumx1)1 << (b))

/* Compute power-of-two modulo bit masks.  */
#define ab_pow2mask1_brnumx1(b) (((ABNumx1)1 << (b)) - 1)
#define ab_pow2mask2_brnumx1(b) ((a) = ((ABNumx1)1 << (b)) - 1)

/* Some very useful convenience routines relating to signs and
   comparisons.  */
#define ab_abs1_brnumx1(b) (((b) < 0) ? -(b) : (b))
#define ab_abs2_brnumx1(a, b) ((a) = ((b) < 0) ? -(b) : (b))
#define ab_min2_brnumx1(b, c) ((ab_cmp2_brnumx1(b, c) < 0) ? (b) : (c))
#define ab_min3_brnumx1(a, b, c) ((a) = ab_min2_brnumx1((b), (c)))
#define ab_max2_brnumx1(b, c) ((ab_cmp2_brnumx1(b, c) > 0) ? (b) : (c))
#define ab_max3_brnumx1(a, b, c) ((a) = ab_max2_brnumx1((b), (c)))

/********************************************************************/
/* Important mathematical bit-wise routines */

ABBitIdx ab_sf_fls1_brnumx1(ABNumx1 a);
void ab_init_fls1_masks_tbl(void);
ABBitIdx ab_sfns_fls1_brnumx1(ABNumx1 a);
ABBitIdx ab_fls1_brnumx1(ABNumx1 a);
#define ab_fls2_brnumx1(a, b) ((a) = ab_fls1_brnumx1(b))
ABBitIdx ab_msbidx1_brnumx1(ABNumx1 a);
#define ab_msbidx2_brnumx1(a, b) ((a) = ab_msbidx1_brnumx1(b))
ABNumx1 ab_fsyshr2_brnumx1(ABNumx1 a, ABBitIdx q);
#define ab_fsyshr3_brnumx1(a, b, c) ((a) = ab_fsyshr2_brnumx1((b), (c)))
ABNumx1 ab_symshr2_brnumx1(ABNumx1 a, ABBitIdx q);
#define ab_symshr3_brnumx1(a, b, c) ((a) = ab_symshr2_brnumx1((b), (c)))

#endif /* not AM_C_ABNumx1 */
