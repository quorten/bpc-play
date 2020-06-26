/* Math subroutines for square roots on built-in C integer data types.
   If this is a ".s.c" file, then it is generalized to all C integer
   data types.  */

#include <stdlib.h>

/*

Template parameters:

* ABNumx1: Scalar number type, base width.

* ABUNumx1: Unsigned scalar number type, base width.

* ABNumx2: Scalar number type, double width.

* ABUNumx2: Unsigned scalar number type, double width.

* ABBitIdx: Integer data type for referencing bit indexes up to the
  width of ABNumx2, i.e. unsigned char.  Must be a built-in C integer
  data type.  Used in base 2 logarithm (shifting) calculations.

* brnumx1: Abbreviated symbol for abstract number, base width, for use
  in higher level type names, functions, etc.

* brunumx1: Abbreviated symbol for unsigned abstract number, base
  width, for use in higher level type names, functions, etc.

* brnumx2: Abbreviated symbol for abstract number, double width, for
  use in higher level type names, functions, etc.

* brunumx2: Abbreviated symbol for unsigned abstract number, double
  width, for use in higher level type names, functions, etc.

* brbitidx: Abbreviated symbol for bit index integer data type, for
  use in higher level type names, functions, etc.

* LOG2_SZ_ABNumx1: Base 2 logarithm of ABNumx1 size in bits.

* ABNumx1_NOSOL: Magic number value to use for "no solution."

* ABUNumx1_MAX: Maximum value for ABUNumx1.

*/

/*

There are plenty of tricks that are used to optimize performance.

* The square root of any 32-bit integer can easily be computed with an
  array of integers squared with only 64K entries.  A simple binary
  search does the trick, the index will correspond to the square root.
  The lookup table itself can be efficiently computed using the
  following formula to compute the next entry from the previous one:

  (x + 1)^2 = x^2 + 2*x*n + 1

  This is quite similar to Bresenham's circle plotting algorithm.

* An approximate square root can be quickly and efficiently computed
  using the find last bit set CPU instruction (`bsr` in x86 CPUs) and
  dividing the number of significant bits by two.  A software fallback
  that does a binary search for the most significant bit in an integer
  is still fairly fast and efficient, even on CPUs that may lack any
  shift instructions whatsoever.

  Please note that when you square the result of the approximate
  square root, it can fall short of the actual number squared by up to
  a factor of four.  So that means the result may be multiplied by a
  factor of sqrt(0.25) = 0.5 compared to the actual value.

  But, by all means, if you only need an order of magnitude estimate,
  this method is plenty fine.

* An iterative bit-guessing square root method is used to compute
  64-bit integer square roots using the following formula for
  stepping:

  (x + n)^2 = x^2 + 2*x*n + n^2

  `x` is the running total of the guessed square root, `n` is the
  current bit value being guessed and tested.  The starting estimate
  is initialized using the find last bit set method to determine the
  approximate square root.

All methods are designed to truncate the result, as is standard with
integer arithmetic, when there is no exact square root solution
available.

*/

/********************************************************************/
/* Fast approximate bit-wise square root routines */

/* Compute a double-width integer approximate square (single-width
   result) root by using find last bit set and dividing the number of
   significant bits by two.  Yes, we shift right to divide by two.  */
ABUNumx1 ab_aprx_sqrt1_brunumx2(ABUNumx2 a)
{
  ABBitIdx num_sig_bits;
  if (a == 0)
    return 0;
  num_sig_bits = ab_fls1_brnumx2(a);
  return (ABUNumx1)1 << (num_sig_bits >> 1);
}

/* Compute a double-width integer approximate square (single-width
   result) root by using find last bit set and dividing the number of
   significant bits by two.  Yes, we shift right to divide by two.  If
   the operand is negative, ABNumx1_NOSOL is returned since there is
   no (real) solution.  */
ABNumx1 ab_aprx_sqrt1_brnumx2(ABNumx2 a)
{
  if (a < 0)
    return ABNumx1_NOSOL;
  return (IVint32)ab_aprx_sqrt_brunumx2((ABUNumx2)a);
}

/* Compute a 64-bit integer approximate square root by using find last
   bit set and dividing the number of significant bits by two.  Yes,
   we shift right to divide by two.  The base 2 logarithm of the
   result is returned.  */
ABBitIdx ab_aprx_sqrt1_log2_brunumx2(ABUNumx2 a)
{
  if (a == 0)
    return 0;
  return ab_fls1_brnumx2(a) >> 1;
}

/********************************************************************/
/* Conventional integer square root routines */

static ABUNumx1 *sqrt_lut_brunumx1 = NULL;

/* Initialize the square root lookup table, used for 32-bit square
   root computations.  Returns one on success, zero on failure.  */
int ab_init_sqrt_lut0_brunumx1(void)
{
  /* (x + 1)^2 = x^2 + 2*x + 1 */
  ABUNumx1 x, x2q, x2;
  ABUNumx1 count = (ABUNumx1)1 << (8 * sizeof(ABUNumx1) / 2);
  sqrt_lut_brunumx1 = (ABUNumx1*)malloc(sizeof(ABUNumx1) * count);
  if (sqrt_lut_brunumx1 == NULL)
    return 0;
  x = 0; x2q = 0; x2 = 1;
  while (x < count) {
    sqrt_lut_brunumx1[x] = x2q;
    x++;
    x2q += x2;
    x2 += 2;
  }
  return 1;
}

/* Destroy the square root lookup table.  */
void ab_destroy_sqrt_lut0_brunumx1(void)
{
  if (sqrt_lut_brunumx1 != NULL)
    free(sqrt_lut_brunumx1);
}

/* Compute a single-width unsigned integer square root using binary
   search on a table of squares "lookup table."  If the square root
   lookup table is not initialized, returns ABNumx1_NOSOL.

   This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
ABNumx1 ab_sqrt_lut1_brunumx1(ABUNumx1 a)
{
  /* ABUNumx1 count = (ABUNumx1)1 << (8 * sizeof(ABUNumx1) / 2) */
  /* TODO FIXME: Previously we used half-width integers here.
     Re-evaluate this.  */
  ABUNumx1 pos = (ABUNumx1)1 << (8 * sizeof(ABUNumx1) / 4);
  ABUNumx1 shift = (ABUNumx1)1 << (8 * sizeof(ABUNumx1) / 8);
  if (sqrt_lut == NULL)
    return ABNumx1_NOSOL;
  while (shift > 1) {
    ABUNumx1 val = sqrt_lut_brunumx1[pos];
    if (a == val)
      return pos;
    else if (a < val) {
      /* Look more carefully to the left.  */
      pos -= shift;
    } else /* (a > val) */ {
      /* Look more carefully to the right.  */
      pos += shift;
    }
    shift >>= 1;
  }
  /* Now do a sequential search on this last segment of size 4: -2,
     -1, 0, +1.  We use -2 because otherwise we can't get all the way
     down to position zero.  */
  pos -= 2;
  shift = pos + 3;
  while (pos != shift) {
    ABUNumx1 val = sqrt_lut_brunumx1[pos];
    if (a == val)
      return pos;
    else if (a < val)
      return pos - 1;
    pos++;
  }
  /* Last iteration is rolled separately so that we don't have to undo
     an excess increment to `pos` in the event of no matches.  */
  if (a < sqrt_lut_brunumx1[pos])
    return pos - 1;
  return pos;
}

/* Compute a single-width integer square root using a binary search on
   a table of squares "lookup table."  If the operand is negative,
   ABNumx1_NOSOL is returned since there is no (real) solution.  If
   the square root lookup table is not initialized, returns
   ABNumx1_NOSOL.

   This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
ABNumx1 ab_sqrt_lut1_brnumx1(ABNumx1 a)
{
  if (a < 0)
    return ABNumx1_NOSOL;
  return ab_sqrt_brunumx1((ABUNumx1)a);
}

/* Compute the square root by successively guessing and testing bits.
   This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
ABUNumx1 ab_sqrt1_brunumx2(ABUNumx2 a)
{
  ABBitIdx pos;
  ABUNumx1 x;
  ABUNumx2 x2;
  if (a == 0)
    return 0;
  if (a <= ABUNumx1_MAX && sqrt_lut_brunumx1 != NULL) {
    return ab_sqrt_lut1_brunumx1((ABUNumx1)a);
  }
  pos = ab_fls1_i64(a) >> 1;
  x = (ABUNumx1)1 << pos;
  x2 = (ABUNumx2)1 << (pos << 1);
  pos--;
  while (pos != (ABBitIdx)-1) {
    ABUNumx1 n = (ABUNumx1)1 << pos; /* n == 2^pos */
    ABUNumx2 n2 = (ABUNumx2)1 << (pos << 1); /* n^2 */
    ABUNumx2 xn2 = (ABUNumx2)x << (pos + 1); /* 2*x*n */
    /* (x + n)^2 = x^2 + 2*x*n + n^2 */
    ABUNumx2 test = x2 + xn2 + n2;
    if (test <= a) {
      x += n;
      x2 = test;
    }
    pos--;
  }
  return x;
}

/* Compute the square root by successively guessing and testing bits.
   If the operand is negative, ABNumx1_NOSOL is returned since there
   is no (real) solution.

   This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
ABNumx1 ab_sqrt1_brnumx2(ABNumx2 a)
{
  if (a < 0)
    return ABNumx1_NOSOL;
  return (ABNumx1)ab_sqrt1_brunumx2((ABUNumx2)a);
}
