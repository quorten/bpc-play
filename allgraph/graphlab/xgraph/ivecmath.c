/* Simple integer vector math subroutines.  A second iteration on my
   previous complicated generics design.  We only support 32-bit
   integers with 64-bit intermediates, two dimensions, and three
   dimensions.

   Fixed-point arithmetic is transparently supported by most
   higher-level vector math operators, i.e. distance to plane, project
   point on plane, intersect, solve system of equations, etc.  This is
   because most of the underlying equations to compute intermediates
   happily cancel out any differences in bits-after-decimal to end up
   with a result with the same number of bits after the decimal, even
   without knowledge of what that is specifically.  In the few areas
   where that is not the case, there are variant subroutines that
   couple shifting right with multiplies.  Here, you specify the shift
   right factor `q` to be equal to the number of bits after the
   decimal and the outputs will be computed with the same number of
   bits-after-decimal.  */

/* TODO FIXME: Improve bit-shifting techniques, when we divide by a
   power of two approximation, always use the base 2 logarithm and bit
   shifting rather than dividing by the literal.

   TODO FIXME: Evaluate.  Should abs() return unsigned?  Double-check
   all arithmetic so that automatic conversions will still give
   correct results, and use casting where warranted.

   TODO FIXME: Add all the various optimized routines for working with
   normalized vectors, rather than just providing the divide routines
   for non-normalized vector operations.

   TODO FIXME: Now that we're done with the proof-of-concept non
   abstract, generics programming version, let's fully implement the
   abstract math version so that we can access our routines across.

   TODO FIXME: Double-check scalar arithmetic tags, make sure we flag
   every place where we may need to use a subroutine (rather than a
   built-in C operator) for arithmetic.  We may even want to define a
   set of subroutines for bit-shifting since with arbitrary precision
   we may need to support a lot of bits.

   TODO FIXME: My `a_along_b` and elim subroutines could be simplified
   if I defined a rational number datatype.  That way, I pass along
   the rational number and determine whether I want to compute the
   quotient immediately or defer.

   TODO FIXME: Speaking of rational numbers, I have the opportunity
   now to fully realize my "cheap normalized" rational numbers data
   type.  I should make sure I have defined subroutines for all my
   numerics trick primitives.

   TODO FIXME: I should revamp all routines so that I return an error
   bit flag instead that can be more cheaply and easily tested by the
   caller, rather than checking all of the vector components for the
   magic number.  This might even entail just testing that the first
   vector component is set to the magic number.

   TODO FIXME: More subroutines:

   * Intersect two plane equations to form a line equation.

   * Line and plane equations with boundaries.

   * Convert a line segment and a triangle to a bounded equation.

   * Convert a line segment or triangle from indices to vertex form.

   TODO: Often times we need to do an identical operation across all
   vector components.  For this we should define a single "for each
   component" macro and use that, rather than repeating the scalar
   code.

   TODO: Definitely we should use symbolic defines for various lengths
   of arrays, i.e. 2D versus 3D length extents.  Just like we do in
   our setup of the 3D scanner API code.

   TODO FIXME: An idea to think about.  For floating point
   compatibility: define a special subroutine for "fast divide by
   two."  Worst comes to worst, we just use a division operator when
   we can't do bit shifting.  Specifically, we only need to rig our
   code for the unsigned subroutine, the signed subroutine is
   unambiguous.

   TODO: Provide approximate methods that use one iteration of
   Newton's Method (or similar in a more "bit-wise" manner) after
   computing approximations using wholly bit-wise techniques.
   Approximate inverse square root works quite well for vector
   normalization.  You can also generalize to n iterations of Newton's
   Method.

*/

/* Only used for `NULL` and `malloc()` for the square root lookup
   table subroutines.  */
#include <stdlib.h>

#include "ivecmath.h"

IVint32 iv_abs_i32(IVint32 a)
{
  return (a < 0) ? -a : a;
}

IVint64 iv_abs_i64(IVint64 a)
{
  return (a < 0) ? -a : a;
}

IVuint8 iv_max2_u8(IVuint8 a, IVuint8 b)
{
  return (a > b) ? a : b;
}

IVuint8 iv_max3_u8(IVuint8 a, IVuint8 b, IVuint8 c)
{
  return iv_max2_u8(iv_max2_u8(a, b), c);
}

/* Get the index of the most significant bit.  To process negative
   numbers, they are inverted and then leading zeros are skipped.

   NOTE: Currently we don't natively implement a 32-bit bit-scan, but
   we define a separate function so we have room to migrate in the
   future.  */
IVuint8 iv_msbidx_i32(IVint32 a)
{
  return soft_fls_i64(iv_abs_i64(a));
}

/* Get the index of the most significant bit.  To process negative
   numbers, they are inverted and then leading zeros are skipped.  */
IVuint8 iv_msbidx_i64(IVint64 a)
{
  return soft_fls_i64(iv_abs_i64(a));
}

/* Shift left abstraction.  Defined since we may not have an
   equivalent C operator for floating point shift left/right, nor do
   most instruction set architectures have an efficient machine
   instruction for that matter.  It's kind of silly because it really
   is almost trivial with floating point.  */
IVint32 iv_shl_i32(IVint32 a, IVuint8 q)
{
  return a << q;
}

/* Shift left abstraction.  Defined since we may not have an
   equivalent C operator for floating point shift left/right, nor do
   most instruction set architectures have an efficient machine
   instruction for that matter.  It's kind of silly because it really
   is almost trivial with floating point.  */
IVint64 iv_shl_i64(IVint64 a, IVuint8 q)
{
  return a << q;
}

/* Shift right abstraction.  Defined since we may not have an
   equivalent C operator for floating point shift left/right, nor do
   most instruction set architectures have an efficient machine
   instruction for that matter.  It's kind of silly because it really
   is almost trivial with floating point.  */
IVint32 iv_shr_i32(IVint32 a, IVuint8 q)
{
  return a >> q;
}

/* Shift right abstraction.  Defined since we may not have an
   equivalent C operator for floating point shift left/right, nor do
   most instruction set architectures have an efficient machine
   instruction for that matter.  It's kind of silly because it really
   is almost trivial with floating point.  */
IVint64 iv_shr_i64(IVint64 a, IVuint8 q)
{
  return a >> q;
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
IVint32 iv_fsyshr2_i32(IVint32 a, IVuint8 q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (a < 0) a += (1 << q) - 1;
  return a >> q;
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
IVint64 iv_fsyshr2_i64(IVint64 a, IVuint8 q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (a < 0) a += (1 << q) - 1;
  return a >> q;
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
IVint32 iv_symshr2_i32(IVint32 a, IVuint8 q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (q != 0 && a < 0) a += (1 << q) - 1;
  return a >> q;
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
IVint64 iv_symshr2_i64(IVint64 a, IVuint8 q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (q != 0 && a < 0) a += (1 << q) - 1;
  return a >> q;
}

/*

Should we implement muldiv_i64 using a 128-bit intermediate?  Here's
how we would do it.

(a, b)  32 + 32 bits wide
(c, d)  32 + 32 bits wide

(a, b) * (c, d) =
                         +128 bits
(c * a),                 +64 bits, 64 wide
   (d * a) + (c * b),    +32 bits, 64 wide
                 (d * b) +0 bits, 64 wide

Don't forget to do soft carries!

Or, better muldiv that avoids overflows.

if d > 32-bit
  if c > 32-bit
    then shift both
  else just do a standard muldiv
if c > 32-bit

*/

/********************************************************************/

IVVec2D_i32 *iv_neg2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = -b->x;
  a->y = -b->y;
  return a;
}

IVVec2D_i32 *iv_add3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = b->x + c->x;
  a->y = b->y + c->y;
  return a;
}

IVVec2D_i32 *iv_sub3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = b->x - c->x;
  a->y = b->y - c->y;
  return a;
}

/* multiply, divide */
IVVec2D_i32 *iv_muldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)((IVint64)b->x * c / d);
  a->y = (IVint32)((IVint64)b->y * c / d);
  return a;
}

/* multiply, divide, 64-bit constants, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec2D_i32 *iv_muldiv4_v2i32_i64(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint64 c, IVint64 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  if (c < 0) { c = -c; d = -d; }
  /* If `c` is too large, shift away all the low bits from both the
     numerator and the denominator, they are insignificant.  */
  if (c >= 0x100000000LL) {
    IVint8 num_sig_bits_c = soft_fls_i64(c);
    IVint8 num_sig_bits_d = iv_msbidx_i64(d);
    IVuint8 shr_div;
    if (num_sig_bits_c - num_sig_bits_d >= 32) {
      /* Fold `c / d` together before multiplying to avoid underflow
	 since `d` is insignificant.  */
      c /= d;
      a->x = (IVint32)((IVint64)b->x * c);
      a->y = (IVint32)((IVint64)b->y * c);
      return a;
    }
    /* else */
    /* Since `c` is too large and `d` is significantly large, shift
       away all the low bits from both the numerator and the
       denominator because they are insignificant.  */
    shr_div = soft_fls_i64(c) - 32;
    /* Avoid asymmetric two's complement behavior.  */
    if (d < 0) d += (1 << shr_div) - 1;
    c >>= shr_div;
    d >>= shr_div;
  }
  /* This should never happen due to our previous logic only acting on
     this path when `d` is significantly large.  */
  /* if (d == 0) {
    /\* No solution.  *\/
    return iv_nosol_v2i32(a);
  } */
  a->x = (IVint32)((IVint64)b->x * c / d);
  a->y = (IVint32)((IVint64)b->y * c / d);
  return a;
}

/* multiply 64-bit, divide 32-bit, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec2D_i32 *iv_muldiv4_v2i32_i64_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				      IVint64 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* N.B.: After some thought about numerical stability, it turns out
     I don't currently have ideas for making a better muldiv_i64_i32
     than what I have for muldiv_i64.  However, the C compiler can
     generate more efficient code on 32-bit CPUs if we specify we're
     dividing a 64-bit by a 32-bit, so we could just copy-paste the
     same code implementation for that paritcular purpose.  */
  return iv_muldiv4_v2i32_i64(a, b, c, (IVint64)d);
}

/* shift left, divide */
IVVec2D_i32 *iv_shldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVuint8 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)(iv_shl_i64((IVint64)b->x, c) / d);
  a->y = (IVint32)(iv_shl_i64((IVint64)b->y, c) / d);
  return a;
}

/* multiply, shift right, with symmetric positive/negative shift
   behavior.  `q` must not be zero.  */
IVVec2D_i32 *iv_mulshr4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)iv_fsyshr2_i64((IVint64)b->x * c, q);
  a->y = (IVint32)iv_fsyshr2_i64((IVint64)b->y * c, q);
  return a;
}

/* shift left */
IVVec2D_i32 *iv_shl3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = iv_shl_i32(b->x, c);
  a->y = iv_shl_i32(b->y, c);
  return a;
}

/* shift right, with symmetric positive/negative shift behavior.  `q`
   must not be zero.  */
IVVec2D_i32 *iv_shr3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVint32 tx = b->x, ty = b->y;
  if (c == 0) {
    a->x = tx;
    a->y = ty;
    return a;
  }
  a->x = iv_fsyshr2_i32(tx, c);
  a->x = iv_fsyshr2_i32(ty, c);
  return a;
}

/* dot product of two vectors */
IVint64 iv_dot2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  return (IVint64)a->x * b->x + (IVint64)a->y * b->y;
}

/* Magnitude squared of a vector.  */
IVint64 iv_magn2q_v2i32(IVVec2D_i32 *a)
{
  return iv_dot2_v2i32(a, a);
}

/* Compute the perpendicular of a vector.  In 2D it's easy, there's
   only one solution (or two if you count both directions).  */
IVVec2D_i32 *iv_perpen2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = -b->y;
  a->y = b->x;
  return a;
}

/* Assign the "no solution" value IVINT32_MIN to all components of the
   given vector.  */
IVVec2D_i32 *iv_nosol_v2i32(IVVec2D_i32 *a)
{
  /* Tags: VEC-COMPONENTS */
  a->x = IVINT32_MIN;
  a->y = IVINT32_MIN;
  return a;
}

/* Test if a vector is equal to the "no solution" vector.  */
IVuint8 iv_is_nosol_v2i32(IVVec2D_i32 *a)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  return (a->x == IVINT32_MIN &&
	  a->y == IVINT32_MIN);
}

/* Test if a vector is equal to zero.  */
IVuint8 iv_is_zero_v2i32(IVVec2D_i32 *a)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  return (a->x == 0 && a->y == 0);
}

/********************************************************************/
/* Integer square root computation routines, for operators that
   require it.  */

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

static IVuint32 *sqrt_lut = NULL;

/* Fallback implementation of find last bit set in software.  */
IVuint8 soft_fls_i64(IVint64 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  IVint64 mask = 0xffffffff00000000LL;
  IVuint8 shift = 0x10;
  IVuint8 pos = 0x20;
  while (shift > 1) {
    if ((a & mask)) {
      /* Look more carefully to the left.  */
      mask <<= shift;
      pos += shift;
      shift >>= 1;
    } else {
      /* Look more carefully to the right.  */
      mask >>= shift;
      pos -= shift;
      shift >>= 1;
    }
  }
  /* Now do a sequential search on this last segment of size 4: +1, 0,
     -1, -2.  We use -2 because otherwise we can't get all the way
     down to bit zero.  */
  mask <<= 1;
  pos += 1;
  /* N.B.: There is nothing to test on the end of the segment because
     we return `pos` either way, so we only need to test the first
     three.  Also, we use != comparison since `shift` may go negative
     and it's unsigned.  */
  shift = pos - 3;
  while (pos != shift) {
    if ((a & mask))
      return pos;
    mask >>= 1;
    pos--;
  }
  return pos;
}

/* Fallback implementation of find last bit set in software, but
   doesn't use any shifting instructions at all!  This is useful if
   your CPU does not have bit-shifting instructions (Gigatron), or it
   doesn't support multi-bit shifts (i.e. 6502).  */
IVuint8 soft_ns_fls_i64(IVint64 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  static const IVint64 masks[64] = {
    0xffffffffffffffffLL,
    0xfffffffffffffffeLL,
    0xfffffffffffffffcLL,
    0xfffffffffffffff8LL,
    0xfffffffffffffff0LL,
    0xffffffffffffffe0LL,
    0xffffffffffffffc0LL,
    0xffffffffffffff80LL,
    0xffffffffffffff00LL,
    0xfffffffffffffe00LL,
    0xfffffffffffffc00LL,
    0xfffffffffffff800LL,
    0xfffffffffffff000LL,
    0xffffffffffffe000LL,
    0xffffffffffffc000LL,
    0xffffffffffff8000LL,
    0xffffffffffff0000LL,
    0xfffffffffffe0000LL,
    0xfffffffffffc0000LL,
    0xfffffffffff80000LL,
    0xfffffffffff00000LL,
    0xffffffffffe00000LL,
    0xffffffffffc00000LL,
    0xffffffffff800000LL,
    0xffffffffff000000LL,
    0xfffffffffe000000LL,
    0xfffffffffc000000LL,
    0xfffffffff8000000LL,
    0xfffffffff0000000LL,
    0xffffffffe0000000LL,
    0xffffffffc0000000LL,
    0xffffffff80000000LL,
    0xffffffff00000000LL,
    0xfffffffe00000000LL,
    0xfffffffc00000000LL,
    0xfffffff800000000LL,
    0xfffffff000000000LL,
    0xffffffe000000000LL,
    0xffffffc000000000LL,
    0xffffff8000000000LL,
    0xffffff0000000000LL,
    0xfffffe0000000000LL,
    0xfffffc0000000000LL,
    0xfffff80000000000LL,
    0xfffff00000000000LL,
    0xffffe00000000000LL,
    0xffffc00000000000LL,
    0xffff800000000000LL,
    0xffff000000000000LL,
    0xfffe000000000000LL,
    0xfffc000000000000LL,
    0xfff8000000000000LL,
    0xfff0000000000000LL,
    0xffe0000000000000LL,
    0xffc0000000000000LL,
    0xff80000000000000LL,
    0xff00000000000000LL,
    0xfe00000000000000LL,
    0xfc00000000000000LL,
    0xf800000000000000LL,
    0xf000000000000000LL,
    0xe000000000000000LL,
    0xc000000000000000LL,
    0x8000000000000000LL,
  };
  static const IVuint8 shifts[6] =
    { 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
  IVuint8 shift_idx = 0;
  IVuint8 pos = 0x20;
  while (shift_idx < 4) {
    if ((a & masks[pos])) {
      /* Look more carefully to the left.  */
      pos += shifts[shift_idx++];
    } else {
      /* Look more carefully to the right.  */
      pos -= shifts[shift_idx++];
    }
  }
  /* Now do a sequential search on this last segment of size 4: +1, 0,
     -1, -2.  We use -2 because otherwise we can't get all the way
     down to bit zero.  */
  pos += 1;
  /* N.B.: There is nothing to test on the end of the segment because
     we return `pos` either way, so we only need to test the first
     three.  Also, we use != comparison since `shift_idx` may go
     negative and it's unsigned.  */
  shift_idx = pos - 3;
  while (pos != shift_idx) {
    if ((a & masks[pos]))
      return pos;
    pos--;
  }
  return pos;
}

/* Initialize the 64K entry square root lookup table, used for 32-bit
   square root computations.  Returns one on success, zero on
   failure.  */
int init_sqrt_lut(void)
{
  /* (x + 1)^2 = x^2 + 2*x + 1 */
  IVuint32 x, x2q, x2;
  sqrt_lut = (IVuint32*)malloc(sizeof(IVuint32) * 0x10000);
  if (sqrt_lut == NULL)
    return 0;
  x = 0; x2q = 0; x2 = 1;
  while (x < 0x10000) {
    sqrt_lut[x] = x2q;
    x++;
    x2q += x2;
    x2 += 2;
  }
  return 1;
}

void destroy_sqrt_lut(void)
{
  if (sqrt_lut != NULL)
    free(sqrt_lut);
}

/* Compute a 32-bit unsigned integer square root using a 64K entry
   lookup table.  If the square root lookup table is not initialized,
   returns IVINT32_MIN.  */
IVint32 iv_sqrt_u32(IVuint32 a)
{
  IVuint16 pos = 0x8000;
  IVuint16 shift = 0x4000;
  if (sqrt_lut == NULL)
    return IVINT32_MIN;
  while (shift > 1) {
    IVuint32 val = sqrt_lut[pos];
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
    IVuint32 val = sqrt_lut[pos];
    if (a == val)
      return pos;
    else if (a < val)
      return pos - 1;
    pos++;
  }
  /* Last iteration is rolled separately so that we don't have to undo
     an excess increment to `pos` in the event of no matches.  */
  if (a < sqrt_lut[pos])
    return pos - 1;
  return pos;
}

/* Compute a 32-bit integer square root using a 64K entry lookup
   table.  If the operand is negative, IVINT32_MIN is returned since
   there is no solution.  If the square root lookup table is not
   initialized, returns IVINT32_MIN.  */
IVint32 iv_sqrt_i32(IVint32 a)
{
  if (a < 0)
    return IVINT32_MIN;
  return iv_sqrt_u32((IVuint32)a);
}

/* Compute a 64-bit integer approximate square root by using find last
   bit set and dividing the number of significant bits by two.  Yes,
   we shift right to divide by two.  If the operand is negative,
   IVINT32_MIN is returned since there is no solution.  */
IVuint32 iv_aprx_sqrt_u64(IVuint64 a)
{
  IVuint8 num_sig_bits;
  if (a == 0)
    return 0;
  num_sig_bits = soft_fls_i64(a);
  return (IVuint32)1 << (num_sig_bits >> 1);
}

IVint32 iv_aprx_sqrt_i64(IVint64 a)
{
  if (a < 0)
    return IVINT32_MIN;
  return (IVint32)iv_aprx_sqrt_u64((IVuint64)a);
}

/* Compute a 64-bit integer approximate square root by using find last
   bit set and dividing the number of significant bits by two.  Yes,
   we shift right to divide by two.  The base 2 logarithm of the
   result is returned.  If the operand is negative, IVINT32_MIN is
   returned since there is no solution.  */
IVuint8 iv_aprx_sqrt_log2_u64(IVuint64 a)
{
  if (a == 0)
    return 0;
  return soft_fls_i64(a) >> 1;
}

/* Improved ("m"= improved) 64-bit approximate square root (slightly
   slower), shift the source number right to get a better estimate of
   the non-power-of-two part of the square root.  This subroutine will
   both underestimate and overestimate the result.
  */
IVuint32 iv_aprx_msqrt_u64(IVuint64 a)
{
  IVuint8 num_sig_bits;
  if (a == 0)
    return 0;
  num_sig_bits = soft_fls_i64(a);
  if ((num_sig_bits & 1) != 0)
    return (IVuint32)(a >> ((num_sig_bits + 1) >> 1));
  /* else */
  /* Try to avoid overestimating by too much by clearing a significant
     bit.  */
  /* TODO FIXME: Do not let shift go negative.  */
  if (num_sig_bits > 0)
    return (IVuint32)((a >> (num_sig_bits >> 1)) &
		      ~(1 << (num_sig_bits - 1)));
  /* else */
  return (IVuint32)(a >> (num_sig_bits >> 1));
}

IVint32 iv_aprx_msqrt_i64(IVint64 a)
{
  if (a < 0)
    return IVINT32_MIN;
  return (IVint32)iv_aprx_msqrt_u64((IVuint64)a);
}

/* This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
IVuint32 iv_sqrt_u64(IVuint64 a)
{
  IVuint8 pos;
  IVuint32 x;
  IVuint64 x2;
  if (a == 0)
    return 0;
  if (a <= 0xffffffff && sqrt_lut != NULL) {
    return iv_sqrt_u32((IVuint32)a);
  }
  pos = soft_fls_i64(a) >> 1;
  x = (IVuint32)1 << pos;
  x2 = (IVuint64)1 << (pos << 1);
  pos--;
  while (pos != (IVuint8)-1) {
    IVuint32 n = (IVuint32)1 << pos; /* n == 2^pos */
    IVuint64 n2 = (IVuint64)1 << (pos << 1); /* n^2 */
    IVuint64 xn2 = (IVuint64)x << (pos + 1); /* 2*x*n */
    /* (x + n)^2 = x^2 + 2*x*n + n^2 */
    IVuint64 test = x2 + xn2 + n2;
    if (test <= a) {
      x += n;
      x2 = test;
    }
    pos--;
  }
  return x;
}

IVint32 iv_sqrt_i64(IVint64 a)
{
  if (a < 0)
    return IVINT32_MIN;
  return (IVint32)iv_sqrt_i64((IVuint64)a);
}

/********************************************************************/
/* Operators that require a square root computation */

IVint32 iv_magnitude_v2i32(IVVec2D_i32 *a)
{
  return iv_sqrt_u64(iv_magn2q_v2i32(a));
}

/* Approximate magnitude, faster but less accurate.  */
IVint32 iv_magn_v2i32(IVVec2D_i32 *a)
{
  return iv_aprx_sqrt_u64(iv_magn2q_v2i32(a));
}

/* Approximate magnitude, faster but less accurate.  The base 2
   logarithm of the result is returned.  */
IVint32 iv_magn_log2_v2i32(IVVec2D_i32 *a)
{
  return iv_aprx_sqrt_log2_u64(iv_magn2q_v2i32(a));
}

/* Doubly approximate vector magnitude determined using find last bit
   set and taking the max of the largest component.  The base 2
   logarithm of the result is returned.  */
IVuint8 iv_amagn_log2_v2i32(IVVec2D_i32 *a)
{
  /* Tags: VEC-COMPONENTS */
  return iv_max2_u8(iv_msbidx_i32(a->x),
		    iv_msbidx_i32(a->y));
}

/* Vector normalization, convert to a Q16.16 fixed-point
   representation.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVNVec2D_i32q16 *iv_normalize2_nv2i32q16_v2i32(IVNVec2D_i32q16 *a,
					       IVVec2D_i32 *b)
{
  IVint32 d = iv_magnitude_v2i32(b);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  return iv_shldiv4_v2i32_i32(a, b, 0x10, d);
}

/* Distance between two points.  */
IVint32 iv_dist2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b)
{
  IVVec2D_i32 t;
  iv_sub3_v2i32(&t, a, b);
  return iv_magnitude_v2i32(&t);
}

/* Approximate distance between two points.  */
IVint32 iv_adist2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b)
{
  IVVec2D_i32 t;
  iv_sub3_v2i32(&t, a, b);
  return iv_magn_v2i32(&t);
}

/* Compute the length of one vector along the direction of another
   vector.

   a_along_b(A, B) = dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVint32 iv_along2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magnitude_v2i32(b);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)(iv_dot2_v2i32(a, b) / d);
}

/* Project a point onto a vector.

   proj_a_on_b(A, B) = B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVVec2D_i32 *iv_proj3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVint32 d;
  d = iv_magnitude_v2i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  return iv_muldiv4_v2i32_i64_i32(a, c, iv_dot2_v2i32(b, c), d);
}

/* Eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVVec2D_i32 *iv_elim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVint32 d;
  IVVec2D_i32 t;
  d = iv_magnitude_v2i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64_i32(&t, c, iv_dot2_v2i32(b, c), d);
  iv_sub3_v2i32(a, b, &t);
  return a;
}

/* Approximate eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVVec2D_i32 *iv_aelim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVint32 d;
  IVVec2D_i32 t;
  d = iv_magn_v2i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64_i32(&t, c, iv_dot2_v2i32(b, c), d);
  iv_sub3_v2i32(a, b, &t);
  return a;
}

/* Project a point onto a line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   P + proj_L

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVPoint2D_i32 *iv_proj3_p2i32_InLine_v2i32(IVPoint2D_i32 *a,
					   IVPoint2D_i32 *b,
					   IVInLine_v2i32 *c)
{
  IVVec2D_i32 t, u;
  iv_sub3_v2i32(&t, b, &c->p0);
  iv_proj3_v2i32(&u, &t, &c->v);
  if (iv_is_nosol_v2i32(&u)) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_add3_v2i32(a, &c->p0, &u);
  return a;
}

/* Shortest path distance squared from point to line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   magnitude^2(L_rel_P - proj_L);

   If there is no solution, IVINT64_MIN is returned.

   TODO FIXME: Add normalized variant.
*/
IVint64 iv_dist2q2_p2i32_InLine_v2i32(IVPoint2D_i32 *a, IVInLine_v2i32 *b)
{
  IVVec2D_i32 t, u;
  iv_sub3_v2i32(&t, a, &b->p0);
  iv_proj3_v2i32(&u, &t, &b->v);
  if (iv_is_nosol_v2i32(&u))
    return IVINT64_MIN;
  return iv_dist2q2_p2i32(&t, &u);
}

/* Shortest path distance from point to line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   magnitude(L_rel_P - proj_L);

   If there is no solution, IVINT32_MIN is returned.

   TODO FIXME: Add normalized variant.
*/
IVint32 iv_dist2_p2i32_InLine_v2i32(IVPoint2D_i32 *a, IVInLine_v2i32 *b)
{
  IVVec2D_i32 t, u;
  iv_sub3_v2i32(&t, a, &b->p0);
  iv_proj3_v2i32(&u, &t, &b->v);
  if (iv_is_nosol_v2i32(&u))
    return IVINT32_MIN;
  return iv_dist2_p2i32(&t, &u);
}

/* Shortest path distance from point to plane (line in 2D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
IVint32 iv_dist2_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVEqs_v2i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magnitude_v2i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)((iv_dot2_v2i32(a, &b->v) - b->offset) / d);
}

/* Approximate shortest path distance from point to plane (line in 2D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
IVint32 iv_adist2_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVEqs_v2i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magn_v2i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)((iv_dot2_v2i32(a, &b->v) - b->offset) / d);
}

/* Shortest path distance from point to plane (line in 2D), normalized
   plane surface normal vector, Q16.16 fixed-point.

   Let L = location vector of point,
       A = normalized plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   dot_product(L, A) - d
*/
IVint32q16 iv_dist2_p2i32_Eqs_nv2i32q16(IVPoint2D_i32 *a, IVEqs_nv2i32q16 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  return (IVint32q16)
    (iv_fsyshr2_i64(iv_dot2_v2i32(a, &b->v), 0x10) - b->offset);
}

/* Alternatively, rather than using a scalar `d`, you can define a
   point in the plane as a vector and subtract it from L before
   computing.

   Let L = location vector of point,
       A = plane surface normal vector,
       P = plane location vector

   L_rel_P = L - P;
   dot_product(L_rel_P, A) / magnitude(A);

   If there is no solution, IVINT32_MIN is returned.

   TODO FIXME: Add normalized variant.
*/
IVint32 iv_dist2_p2i32_NRay_v2i32(IVPoint2D_i32 *a, IVNLine_v2i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d;
  IVVec2D_i32 l_rel_p;
  d = iv_magnitude_v2i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  iv_sub3_v2i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v2i32(&l_rel_p, &b->v) / d);
}

/* Approximate variant of distance to point in plane, like the
   subroutine `iv_dist2_p2i32_NRay_v2i32()`.

   TODO FIXME: Add normalized variant.  */
IVint32 iv_adist2_p2i32_NRay_v2i32(IVPoint2D_i32 *a, IVNLine_v2i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d;
  IVVec2D_i32 l_rel_p;
  d = iv_magn_v2i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  iv_sub3_v2i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v2i32(&l_rel_p, &b->v) / d);
}

/********************************************************************/

/* Distance squared between two points.  */
IVint64 iv_dist2q2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b)
{
  IVVec2D_i32 t;
  iv_sub3_v2i32(&t, a, b);
  return iv_magn2q_v2i32(&t);
}

/* Project a point to a plane along the perpendicular:

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   L - A * dist_point_to_plane(L, A, d) / magnitude(A)

   Because we are dividing by magnitude twice, the division quantity
   is squared so we therefore can avoid computing the square root and
   simplify as follows:

   L - A * (dot_product(L, A) - d) / magnitude^2(A)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVPoint2D_i32 *iv_proj3_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					IVEqs_v2i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 t;
  IVint64 d;
  d = iv_magn2q_v2i32(&c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64(&t, &c->v, iv_dot2_v2i32(b, &c->v) - c->offset, d);
  return iv_sub3_v2i32(a, b, &t);
}

/* Project a point to a plane along the perpendicular:

   Let L = location vector of point,
       A = plane surface normal vector,
       P = plane location vector

   L - A * dist_point_to_plane(L, P, A) / magnitude(A)

   Because we are dividing by magnitude twice, the division quantity
   is squared so we therefore can avoid computing the square root and
   simplify as follows:

   L_rel_P = L - P;
   L - A * dot_product(L_rel_P, A) / magnitude^2(A);

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.
*/
IVPoint2D_i32 *iv_proj3_p2i32_NLine_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					  IVNLine_v2i32 *c)
{
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  IVint64 d;
  iv_sub3_v2i32(&l_rel_p, b, &c->p0);
  d = iv_magn2q_v2i32(&c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64(&t, &c->v, iv_dot2_v2i32(&l_rel_p, &c->v), d);
  return iv_sub3_v2i32(a, b, &t);
}

/* Bi-directional intersection of line with plane:

   Let D = line direction vector,
       L = line location vector,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   Fully simplified equation:

   L - D * (dot_product(L, A) - d) / dot_product(D, A)

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   NOTE: Often times it is useful to compute `dot_product(D, A)` in
   advance since this can be used as a "quality factor" to check if
   the angle of intersection is well-formed.  Because of this, there a
   separate solver subroutine to handle a precomputed `dot_product(D,
   A)`, to avoid redundantly computing it again.
*/
IVPoint2D_i32 *iv_isect3_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					  IVInLine_v2i32 *b,
					  IVEqs_v2i32 *c)
{
  IVint64 d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  return iv_p2isect4_InLine_Eqs_v2i32(a, b, c, d);
}

/* Second part of `iv_isect3_InLine_Eqs_v2i32()`.  `d` is
   `dot_product(D, A)` and it must not be zero.  */
IVPoint2D_i32 *iv_p2isect4_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVEqs_v2i32 *c,
					    IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 t;
  iv_muldiv4_v2i32_i64(&t, &b->v,
		       iv_dot2_v2i32(&b->p0, &c->v) - c->offset, d);
  return iv_sub3_v2i32(a, &b->p0, &t);
}

/* Bi-directional intersection of line with plane:

   Let D = line direction vector,
       L = line location vector,
       A = plane surface normal vector,
       P = plane location vector

   Fully simplified equations:

   L_rel_P = L - P;
   result = L - D * dot_product(L_rel_P, A) / dot_product(D, A);

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   NOTE: Often times it is useful to compute `dot_product(D, A)` in
   advance since this can be used as a "quality factor" to check if
   the angle of intersection is well-formed.  Because of this, there a
   separate solver subroutine to handle a precomputed `dot_product(D,
   A)`, to avoid redundantly computing it again.
*/
IVPoint2D_i32 *iv_isect3_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVNLine_v2i32 *c)
{
  IVint64 d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  return iv_p2isect4_InLine_NLine_v2i32(a, b, c, d);
}

/* Second part of `iv_isect3_InLine_NLine_v2i32()`.  `d` is
   `dot_product(D, A)` and it must not be zero.  */
IVPoint2D_i32 *iv_p2isect4_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					      IVInLine_v2i32 *b,
					      IVNLine_v2i32 *c,
					      IVint64 d)
{
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  iv_sub3_v2i32(&l_rel_p, &b->p0, &c->p0);
  iv_muldiv4_v2i32_i64(&t, &b->v, iv_dot2_v2i32(&l_rel_p, &c->v), d);
  return iv_sub3_v2i32(a, &b->p0, &t);
}

/* Directional intersection of ray with plane:

   Let D = ray direction vector,
       L = ray location vector,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   Fully simplified equation:

   numer = -(dot_product(L, A) - d);
   denom = dot_product(D, A);
   if (numer / denom < 0) then no solution;
   L + D * numer / denom;

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint2D_i32 *iv_isect3_Ray_Eqs_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
				       IVEqs_v2i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  n = -iv_dot2_v2i32(&b->p0, &c->v) + c->offset;
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64(&t, &b->v, n, d);
  return iv_add3_v2i32(a, &b->p0, &t);
}

/* Directional intersection of ray with plane:

   Let D = ray direction vector,
       L = ray location vector,
       A = plane surface normal vector,
       P = plane location vector

   Fully simplified equations:

   L_rel_P = L - P;
   numer = -dot_product(L_rel_P, A);
   denom = dot_product(D, A);
   if (numer / denom < 0) then no solution;
   result = L + D * numer / denom;

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint2D_i32 *iv_isect3_Ray_NLine_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
					 IVNLine_v2i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_sub3_v2i32(&l_rel_p, &b->p0, &c->p0);
  n = -iv_dot2_v2i32(&l_rel_p, &c->v);
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  iv_muldiv4_v2i32_i64(&t, &b->v, n, d);
  return iv_add3_v2i32(a, &b->p0, &t);
}

/* Quality factor check after computing a dot product, and typically
   before computibng a line-plane intersection.  This is computed as
   follows:

   Let A = line direction vector,
       B = plane surface normal vector,
       D = dot_product(A, B)

   D / (magnitude(A) * magnitude(B))

   If either vector magnitude is zero, the returned quality factor is
   zero.

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated square roots, sometimes the result is greater
   than 1.0 in Q16.16.  */
IVint32q16 iv_postqualfac_dot3_i32q16_v2i32(IVVec2D_i32 *a,
					    IVVec2D_i32 *b,
					    IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 magn_a, magn_b;
  IVint32q16 norm_d;
  magn_a = iv_magnitude_v2i32(a);
  if (magn_a == 0)
    return 0;
  magn_b = iv_magnitude_v2i32(b);
  if (magn_b == 0)
    return 0;
  if (d >= 0x80000000LL) {
    /* Okay, this is tricky since we don't have much headroom due to
       the 64-bit numerator.  Divide by the smallest factor and the
       remaining factor is guaranteed to be within the headroom of
       IVint32.  So then we can do a standard multiply-divide to
       obtain the fixed-point result.  */
    if (magn_b < magn_a) {
      d /= magn_b;
      norm_d = (IVint32q16)((d << 0x10) / magn_a);
    } else {
      d /= magn_a;
      norm_d = (IVint32q16)((d << 0x10) / magn_b);
    }
  } else {
    IVint64 magn_a_b = (IVint64)magn_a * magn_b;
    norm_d = (IVint32q16)((d << 0x10) / magn_a_b);
  }
  return norm_d;
}

/* Approximate quality factor check after computing a dot product, and
   typically before computibng a line-plane intersection.  This is
   computed as follows:

   Let A = line direction vector,
       B = plane surface normal vector,
       D = dot_product(A, B)

   D / (magnitude(A) * magnitude(B))

   If either vector magnitude is zero, the returned quality factor is
   zero.

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated approximate square roots, sometimes the
   result is greater than 1.0 in Q16.16, often times up to a factor of
   3.0.

   To be fast, use approximate magnitude instead.  Operating directly
   on the results of find last bit set is fastest since you can add
   rather than multiply.  */
IVint32q16 iv_apostqualfac_dot3_i32q16_v2i32(IVVec2D_i32 *a,
					     IVVec2D_i32 *b,
					     IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint64 magn2q_a, magn2q_b;
  IVint8 shift_factor;
  magn2q_a = iv_magn2q_v2i32(a);
  if (magn2q_a == 0)
    return 0;
  magn2q_b = iv_magn2q_v2i32(b);
  if (magn2q_b == 0)
    return 0;
  shift_factor = 0x10 - (((IVint8)soft_fls_i64(magn2q_a) +
			  (IVint8)soft_fls_i64(magn2q_b))
			 >> 1);
  if (shift_factor < 0)
    return (IVint32q16)(d >> -shift_factor);
  /* else */
  return (IVint32q16)(d << shift_factor);
}

/********************************************************************/

/* Convert (reformat) the scalar origin offset of an Eqs
   representational form to point vector representational form.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.  */
IVVec2D_i32 *iv_rf2_v2i32_Eqs_v2i32(IVVec2D_i32 *a, IVEqs_v2i32 *b)
{
  IVint64 d = iv_magn2q_v2i32(&b->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v2i32(a);
  }
  return iv_muldiv4_v2i32_i64(a, &b->v, b->offset, d);
}

/* Convert (reformat) Eqs representational form to NLine.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   TODO FIXME: Add normalized variant.  */
IVNLine_v2i32 *iv_rf2_NLine_Eqs_v2i32(IVNLine_v2i32 *a, IVEqs_v2i32 *b)
{
  /* Convert the origin offset to a point.  */
  iv_rf2_v2i32_Eqs_v2i32(&a->p0, b);
  if (iv_is_nosol_v2i32(&a->p0)) {
    /* No solution.  */
    iv_nosol_v2i32(&a->v);
    return a;
  }
  /* Copy the perpendicular vector.  */
  a->v = b->v;
  return a;
}

/* Convert (reformat) NLine representational form to Eqs.  */
IVEqs_v2i32 *iv_rf2_Eqs_NLine_v2i32(IVEqs_v2i32 *a, IVNLine_v2i32 *b)
{
  IVVec2D_i32 t;
  /* Copy the perpendicular vector.  */
  a->v = b->v;
  /* Essentially, computing the origin offset is computing the
     distance from the origin point to the plane, and then multiplying
     by the length of the surface normal vector.  Therefore, we
     happily avoid computing a square root in a magnitude computation.

     L = 0;
     L_rel_P = L - P;
     dot_product(L_rel_P, A) / magnitude(A)
     = dot_product(-P, A) / magnitude(A);

     Multiply by magnitude(A):
     dot_product(-P, A)
  */
  iv_neg2_v2i32(&t, &b->p0);
  a->offset = iv_dot2_v2i32(&t, &b->v);
  return a;
}

/* Convert (reformat) InLine representational form to NLine.  */
IVNLine_v2i32 *iv_rf2_NLine_InLine_v2i32(IVNLine_v2i32 *a, IVInLine_v2i32 *b)
{
  /* Copy the location.  */
  a->p0 = b->p0;
  /* Compute the perpendicular vector to use for the NLine.  */
  iv_perpen2_v2i32(&a->v, &b->v);
  return a;
}

/* Convert (reformat) NLine representational form to InLine.  */
IVInLine_v2i32 *iv_rf2_InLine_NLine_v2i32(IVInLine_v2i32 *a, IVNLine_v2i32 *b)
{
  /* Exactly the same code as the reverse conversion, just different
     types.  */
  return (IVInLine_v2i32*)
    iv_rf2_NLine_InLine_v2i32((IVNLine_v2i32*)a, (IVInLine_v2i32*)b);
}

/* Solve a system of two simple linear equations, i.e. Ax = b format.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_Eqs_v2i32(IVPoint2D_i32 *a, IVSys2_Eqs_v2i32 *b)
{
  IVNLine_v2i32 nline;
  IVInLine_v2i32 in_line;
  /* Reformat the first equation into a InLine equation.  */
  iv_rf2_NLine_Eqs_v2i32(&nline, &b->d[0]);
  iv_rf2_InLine_NLine_v2i32(&in_line,  &nline);
  /* Now intersect the in_line with the plane (line in 2D).  */
  return iv_isect3_InLine_Eqs_v2i32(a, &in_line, &b->d[1]);
}

/* Solve a system of two "surface-normal" perpendicular vector linear
   equations.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_NLine_v2i32(IVPoint2D_i32 *a,
					IVSys2_NLine_v2i32 *b)
{
  IVInLine_v2i32 in_line;
  /* Reformat the first equation into an InLine equation.  */
  iv_rf2_InLine_NLine_v2i32(&in_line, &b->d[0]);
  /* Now intersect the InLine with the plane (line in 2D).  */
  return iv_isect3_InLine_NLine_v2i32(a, &in_line, &b->d[1]);
}

/* Solve a system of two InLine vector linear equations.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_InLine_v2i32(IVPoint2D_i32 *a,
					 IVSys2_InLine_v2i32 *b)
{
  IVNLine_v2i32 nline;
  /* Reformat the second equation into an NLine equation.  */
  iv_rf2_InLine_NLine_v2i32(&nline, &b->d[1]);
  /* Now intersect the InLine with the plane (line in 2D).  */
  return iv_isect3_InLine_NLine_v2i32(a, &b->d[0], &nline);
}

/* Quality factor check on system of equations before solving.  This
   is computed using the dot product quality factor on the plane
   surface normal vectors.  The absolute value of the dot product is
   taken so that the result does not go negative.  */
IVint32q16 iv_prequalfac_i32q16_s2_Eqs_v2i32(IVSys2_Eqs_v2i32 *a)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 vt;
  IVint64 vt_dot_v2;
  IVint32 magn_vt, magn_v2;
  IVint32q16 norm_vt_dot_v2;
  iv_perpen2_v2i32(&vt, &a->d[0].v);
  vt_dot_v2 = iv_abs_i64(iv_dot2_v2i32(&vt, &a->d[1].v));
  return iv_postqualfac_dot3_i32q16_v2i32(&vt, &a->d[1].v, vt_dot_v2);
}

/* Approximate quality factor check on system of equations before
   solving.  This is computed using the dot product approximate
   quality factor on the plane surface normal vectors.  The absolute
   value of the dot product is taken so that the result does not go
   negative.  */
IVint32q16 iv_aprequalfac_i32q16_s2_Eqs_v2i32(IVSys2_Eqs_v2i32 *a)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec2D_i32 vt;
  IVint64 vt_dot_v2;
  IVint64 magn2q_vt, magn2q_v2;
  IVint8 shift_factor;
  iv_perpen2_v2i32(&vt, &a->d[0].v);
  vt_dot_v2 = iv_abs_i64(iv_dot2_v2i32(&vt, &a->d[1].v));
  return iv_apostqualfac_dot3_i32q16_v2i32(&vt, &a->d[1].v, vt_dot_v2);
}

/********************************************************************/

/* Multiply, shift right two MxN integer matrices, with symmetric
   positive/negative shift behavior.  After each element
   multiplication, the result is shifted right by `q` bits before
   adding to the accumulator.  The output matrix must be
   pre-allocated.

   If there is no solution, the resulting matrix entries are all set
   to IVINT32_MIN.  */
IVMatNxM_i32 *iv_mulshr4_mnxm_i32(IVMatNxM_i32 *a,
				  IVMatNxM_i32 *b, IVMatNxM_i32 *c,
				  IVuint8 q)
{
  IVuint16 inner = b->width;
  IVuint16 a_width = a->width, a_height = a->height;
  IVint32 *a_d = a->d, *b_d = b->d, *c_d = c->d;

  if (inner != c->height ||
      b->height != a_height ||
      c->width != a_width) {
    /* No solution, matrix dimension mismatch.  */
    IVuint32 a_size = a_height * a_width;
    IVuint32 i;
    for (i = 0; i < a_size; i++) {
      a_d[i] = IVINT32_MIN;
    }
    return a;
  }

  {
    IVuint16 i, j, k;
    IVuint16 a_width_i = 0; /* (a_width * i) */
    IVuint16 inner_i = 0; /* (inner * i) */
    for (i = 0; i < a_height; i++) {
      for (j = 0; j < a_width; j++) {
	IVint32 accum = 0;
	IVuint16 a_width_k = 0; /* (a_width * k) */
	for (k = 0; k < inner; k++) {
	  IVint64 intermed =
	    (IVint64)b_d[inner_i+k] * c_d[a_width_k+j];
	  accum += iv_symshr2_i64(intermed, q);
	  a_width_k += a_width;
	}
	a_d[a_width_i+j] = accum;
      }
      a_width_i += a_width;
      inner_i += inner;
    }
  }

  return a;
}

/* Transpose an integer matrix.  The output matrix must be
   pre-allocated.

   If there is a dimension mismatch, the resulting matrix entries are
   all set to IVINT32_MIN.  */
IVMatNxM_i32 *iv_xpose2_mnxm_i32(IVMatNxM_i32 *a, IVMatNxM_i32 *b)
{
  IVuint16 a_width = a->width, a_height = a->height;
  IVint32 *a_d = a->d, *b_d = b->d;

  if (a_width != b->height || a_height != b->width) {
    /* No solution, matrix dimension mismatch.  */
    IVuint32 a_size = a_height * a_width;
    IVuint32 i;
    for (i = 0; i < a_size; i++) {
      a_d[i] = IVINT32_MIN;
    }
    return a;
  }

  {
    IVuint16 i, j;
    IVuint16 a_width_i = 0; /* (a_width * i) */
    for (i = 0; i < a_height; i++) {
      IVuint16 a_height_j = 0; /* (a_height * j) */
      for (j = 0; j < a_width; j++) {
	a_d[a_width_i+j] = b_d[a_height_j+i];
	a_height_j += a_height;
      }
      a_width_i += a_width;
    }
  }

  return a;
}

/********************************************************************/
/* Statistical routines */

/* Compute the mean point coordinate of a set of points.  Division
   is performed at the very end as follows:

   sum(i = 1 to n, x_i) / n

   The intermediate sum computation uses the same precision as the
   final result.  */
IVVec2D_i32 *iv_mean2_v2i32_arn_v2i32(IVPoint2D_i32 *a,
				      IVPoint2D_i32_array *data)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVPoint2D_i32 *data_d = data->d;
  IVPoint2D_i32 w = { 0, 0 };
  IVuint16 data_len = data->len;
  IVuint16 i;

  for (i = 0; i < data_len; i++) {
    w.x += data_d[i].x;
    w.y += data_d[i].y;
  }
  w.x /= data_len;
  w.y /= data_len;

  *a = w;
  return a;
}

/* Set up system of equations to compute a linear regression of the "y
   = m*x + b" representational form.  When the system is solved, the
   result vector format is `[b, m]`.  The equations are formatted for
   solving in Q16.16 fixed-point format since you'll need a
   fixed-point number for the fractional slope.

   `q` = shift right factor, bits after decimal for fixed-point
   operands

   If there is a memory allocation error, the system of equation's
   entries are all set to IVINT32_MIN (and IVINT64_MIN).  */
IVSys2_Eqs_v2i32q16 *iv_pack_linreg3_s2_Eqs_v2i32q16_arn_v2i32
  (IVSys2_Eqs_v2i32q16 *sys, IVPoint2D_i32_array *data, IVuint8 q)
{
  IVPoint2D_i32 *data_d = data->d;
  IVuint16 data_len = data->len;
  IVMatNxM_i32 mat_a; /* Matrix A */
  IVMatNxM_i32 mat_a_t; /* Matrix A^T */
  IVMatNxM_i32 col_b; /* Column vector b */
  IVMatNxM_i32 mat_a_t_a; /* Matrix A^T * A */
  IVMat2x2_i32 mat_a_t_a_stor; /* Allocation for Matrix A^T * A */
  IVMatNxM_i32 col_a_t_b; /* Column vector A^T * b */
  IVVec2D_i32 col_a_t_b_stor; /* Allocation for Column vector A^T * b */
  IVuint16 i;

  /* Pre-allocate all intermediate data structures.  */
  mat_a.d = NULL;
  mat_a_t.d = NULL;
  col_b.d = NULL;

  mat_a.width = 2;
  mat_a.height = data_len;
  mat_a.d = (IVint32*)malloc(sizeof(IVint32) * mat_a.width * mat_a.height);
  if (mat_a.d == NULL)
    goto dirty_cleanup;

  mat_a_t.width = mat_a.height;
  mat_a_t.height = mat_a.width;
  mat_a_t.d =
    (IVint32*)malloc(sizeof(IVint32) * mat_a_t.width * mat_a_t.height);
  if (mat_a_t.d == NULL)
    goto dirty_cleanup;

  col_b.width = 1;
  col_b.height = data_len;
  col_b.d =
    (IVint32*)malloc(sizeof(IVint32) * col_b.width * col_b.height);
  if (col_b.d == NULL)
    goto dirty_cleanup;

  mat_a_t_a.width = 2;
  mat_a_t_a.height = 2;
  mat_a_t_a.d = mat_a_t_a_stor.d;
  col_a_t_b.width = 1;
  col_a_t_b.height = 2;
  col_a_t_b.d = (IVint32*)&col_a_t_b_stor;

  /* Pack into the coefficient matrix `A`.  */
  for (i = 0; i < data_len; i++) {
    mat_a.d[mat_a.width*i+0] = 1 << q;
    mat_a.d[mat_a.width*i+1] = data_d[i].x;
  }
  /* Compute `A^T`.  */
  iv_xpose2_mnxm_i32(&mat_a_t, &mat_a);

  /* Pack into the coefficient matrix `b`.  */
  for (i = 0; i < data_len; i++) {
    col_b.d[i] = data_d[i].y;
  }

  /* Compute `A^T * A`.  */
  iv_mulshr4_mnxm_i32(&mat_a_t_a, &mat_a_t, &mat_a, q);

  /* Compute `A^T * b`.  */
  iv_mulshr4_mnxm_i32(&col_a_t_b, &mat_a_t, &col_b, q);

  /* Pack into IVSys2_Eqs_v2i32 representational form.  */
  sys->d[0].v.x = mat_a_t_a.d[0];
  sys->d[0].v.y = mat_a_t_a.d[1];
  sys->d[0].offset = col_a_t_b.d[0];
  sys->d[1].v.x = mat_a_t_a.d[2];
  sys->d[1].v.y = mat_a_t_a.d[3];
  sys->d[1].offset = col_a_t_b.d[1];

  /* Here's what we're doing here.  Since we'll need a fixed-point
     solution, our equation solver will operate with a Q16.16 decimal
     point.  We use the "cheap normalization" technique.  For large
     integers, they can just be passed in without modification and it
     will all work, since in a system of equations you can divide all
     entries by the same number and get the same result.  However, if
     our inputs are small integers and we input them as-is, some
     values may get shifted to zero in the computations, so we
     adaptively shift all entries right based off of an entry that is
     similar to the median value of the entries.  */
  {
    IVint32 test_value = sys->d[0].v.y;
    IVuint8 shr_div = iv_msbidx_i32(test_value);
    IVuint8 num_shr;
    if (shr_div > 0x10)
      shr_div = 0x10;
    num_shr = 0x10 - shr_div;
    sys->d[0].v.x <<= num_shr;
    sys->d[0].v.y <<= num_shr;
    sys->d[1].v.x <<= num_shr;
    sys->d[1].v.y <<= num_shr;
    /* `offset` must be shifted twice as many bits.  */
    num_shr = 0x20 - shr_div;
    sys->d[0].offset <<= num_shr;
    sys->d[1].offset <<= num_shr;
  }

  /* Cleanup.  */
  free (mat_a.d);
  free (mat_a_t.d);
  free (col_b.d);

  return sys;

 dirty_cleanup:
  iv_nosol_v2i32(&sys->d[0].v);
  sys->d[0].offset = IVINT64_MIN;
  iv_nosol_v2i32(&sys->d[1].v);
  sys->d[1].offset = IVINT64_MIN;
  if (mat_a.d != NULL) free (mat_a.d);
  if (mat_a_t.d != NULL) free (mat_a_t.d);
  if (col_b.d != NULL) free (col_b.d);
  return sys;
}

/********************************************************************/
/********************************************************************/
/* Now we copy-pasta code to reimplement everything all over again in
   3D.  Copying code solely for the sake of making debugging easy in
   the short-term.  Long-term I should use a code generator that emits
   multiple C source code file, so that we can be debugger-friendly.
   Rather than, say, do everything with C macros and the C
   preprocessor.  */

IVVec3D_i32 *iv_neg2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = -b->x;
  a->y = -b->y;
  a->z = -b->z;
  return a;
}

IVVec3D_i32 *iv_add3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = b->x + c->x;
  a->y = b->y + c->y;
  a->z = b->z + c->z;
  return a;
}

IVVec3D_i32 *iv_sub3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = b->x - c->x;
  a->y = b->y - c->y;
  a->z = b->z - c->z;
  return a;
}

/* multiply, divide */
IVVec3D_i32 *iv_muldiv4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint32 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)((IVint64)b->x * c / d);
  a->y = (IVint32)((IVint64)b->y * c / d);
  a->z = (IVint32)((IVint64)b->z * c / d);
  return a;
}

/* multiply, divide, 64-bit constants, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec3D_i32 *iv_muldiv4_v3i32_i64(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint64 c, IVint64 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  if (c < 0) { c = -c; d = -d; }
  /* If `c` is too large, shift away all the low bits from both the
     numerator and the denominator, they are insignificant.  */
  if (c >= 0x100000000LL) {
    IVint8 num_sig_bits_c = soft_fls_i64(c);
    IVint8 num_sig_bits_d = iv_msbidx_i64(d);
    IVuint8 shr_div;
    if (num_sig_bits_c - num_sig_bits_d >= 32) {
      /* Fold `c / d` together before multiplying to avoid underflow
	 since `d` is insignificant.  */
      c /= d;
      a->x = (IVint32)((IVint64)b->x * c);
      a->y = (IVint32)((IVint64)b->y * c);
      a->z = (IVint32)((IVint64)b->z * c);
      return a;
    }
    /* else */
    /* Since `c` is too large and `d` is significantly large, shift
       away all the low bits from both the numerator and the
       denominator because they are insignificant.  */
    shr_div = soft_fls_i64(c) - 32;
    /* Avoid asymmetric two's complement behavior.  */
    if (d < 0) d += (1 << shr_div) - 1;
    c >>= shr_div;
    d >>= shr_div;
  }
  /* This should never happen due to our previous logic only acting on
     this path when `d` is significantly large.  */
  /* if (d == 0) {
    /\* No solution.  *\/
    return iv_nosol_v3i32(a);
  } */
  a->x = (IVint32)((IVint64)b->x * c / d);
  a->y = (IVint32)((IVint64)b->y * c / d);
  a->z = (IVint32)((IVint64)b->z * c / d);
  return a;
}

/* multiply 64-bit, divide 32-bit, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec3D_i32 *iv_muldiv4_v3i32_i64_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				      IVint64 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* N.B.: After some thought about numerical stability, it turns out
     I don't currently have ideas for making a better muldiv_i64_i32
     than what I have for muldiv_i64.  However, the C compiler can
     generate more efficient code on 32-bit CPUs if we specify we're
     dividing a 64-bit by a 32-bit, so we could just copy-paste the
     same code implementation for that paritcular purpose.  */
  return iv_muldiv4_v3i32_i64(a, b, c, (IVint64)d);
}

/* shift left, divide */
IVVec3D_i32 *iv_shldiv4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVuint8 c, IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)(iv_shl_i64((IVint64)b->x, c) / d);
  a->y = (IVint32)(iv_shl_i64((IVint64)b->y, c) / d);
  a->z = (IVint32)(iv_shl_i64((IVint64)b->z, c) / d);
  return a;
}

/* multiply, shift right, with symmetric positive/negative shift
   behavior.  `q` must not be zero.  */
IVVec3D_i32 *iv_mulshr4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint32 c, IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)iv_fsyshr2_i64((IVint64)b->x * c, q);
  a->y = (IVint32)iv_fsyshr2_i64((IVint64)b->y * c, q);
  a->z = (IVint32)iv_fsyshr2_i64((IVint64)b->z * c, q);
  return a;
}

/* shift left */
IVVec3D_i32 *iv_shl3_v3i32_u32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = iv_shl_i32(b->x, c);
  a->y = iv_shl_i32(b->y, c);
  a->z = iv_shl_i32(b->z, c);
  return a;
}

/* shift right, with symmetric positive/negative shift behavior.  `q`
   must not be zero.  */
IVVec3D_i32 *iv_shr3_v3i32_u32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVint32 tx = b->x, ty = b->y, tz = b->z;
  if (c == 0) {
    a->x = tx;
    a->y = ty;
    a->z = tz;
    return a;
  }
  a->x = iv_fsyshr2_i32(tx, c);
  a->y = iv_fsyshr2_i32(ty, c);
  a->z = iv_fsyshr2_i32(tz, c);
  return a;
}

/* dot product of two vectors */
IVint64 iv_dot2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  return (IVint64)a->x * b->x + (IVint64)a->y * b->y + (IVint64)a->z * b->z;
}

/* Magnitude squared of a vector.  */
IVint64 iv_magn2q_v3i32(IVVec3D_i32 *a)
{
  return iv_dot2_v3i32(a, a);
}

/*
   cross_product(A, B) =
               [ A_y * B_z - A_z * B_y,
                 A_z * B_x - A_x * B_z,
                 A_x * B_y - A_y * B_x ]
*/
IVVec3D_i32 *iv_crossproddiv4_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c,
				    IVint32 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)(((IVint64)b->y * c->z -
		    (IVint64)b->z * c->y) / d);
  a->y = (IVint32)(((IVint64)b->z * c->x -
		    (IVint64)b->x * c->z) / d);
  a->z = (IVint32)(((IVint64)b->x * c->y -
		    (IVint64)b->y * c->x) / d);
  return a;
}

/* Cross product, shift right, with symmetric positive/negative shift
   behavior.  */
IVVec3D_i32 *iv_crossprodshr4_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c,
				    IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  a->x = (IVint32)iv_symshr2_i64((IVint64)b->y * c->z -
				 (IVint64)b->z * c->y, q);
  a->y = (IVint32)iv_symshr2_i64((IVint64)b->z * c->x -
				 (IVint64)b->x * c->z, q);
  a->z = (IVint32)iv_symshr2_i64((IVint64)b->x * c->y -
				 (IVint64)b->y * c->x, q);
  return a;
}

/* Compute the adaptive shift right "cheap normalization" value to use
   with a cross product to limit the resulting vector components to 16
   significant bits, approximately Q16.16 format.  */
IVuint8 iv_cnf_crossprod2_v3i32(IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  IVuint8 num_sig_bits = iv_amagn_log2_v3i32(b) + iv_amagn_log2_v3i32(c);
  IVuint8 shift_factor = (num_sig_bits > 0x10) ? num_sig_bits - 0x10 : 0;
  return shift_factor;
}

/* Cross product with adaptive shift right using "cheap normalization"
   technique.  A shift right factor is determined to limit the
   resulting vector components to 16 significant bits.  */
IVVec3D_i32 *iv_crossprod3_cn_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVuint8 shift_factor = iv_cnf_crossprod2_v3i32(b, c);
  return iv_crossprodshr4_v3i32(a, b, c, shift_factor);
}

/* Quality factor check after computing a surface normal vector via a
   cross product.

   Let A = InPlane vector 1,
       B = InPlane vector 2,
       C = cross product result vector

   quality = sin(theta) = magnitude(C) / (magnitude(A) * magnitude(B))

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated square roots, sometimes the result is greater
   than 1.0 in Q16.16.  */
IVint32q16 iv_postqualfac_cps4_i32q16_v3i32
  (IVVec3D_i32 *c, IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 wqualfac = iv_magnitude_v3i32(c);
  wqualfac = (IVint32)((IVint64)(wqualfac << q) / iv_magnitude_v3i32(a));
  wqualfac = (IVint32)((IVint64)(wqualfac << 0x10) / iv_magnitude_v3i32(b));
  return (IVint32q16)wqualfac;
}

/* Approximate quality factor check after computing a surface normal
   vector via a cross product.

   Let A = InPlane vector 1,
       B = InPlane vector 2,
       C = cross product result vector

   quality = sin(theta) = magnitude(C) / (magnitude(A) * magnitude(B))

   We can use bit-shifting techniques instead to compute the
   approximate magnitudes and approximate quality factor.  We can also
   save the final approximate square root computation until last.  And
   finally, we can use "cheap normalization" to avoid division when
   computing the fixed-point quality factor.

   Because of the approximate calculations, the result may smaller
   than the actual result by a factor of two.  */
IVint32q16 iv_apostqualfac_cps4_i32q16_v3i32
  (IVVec3D_i32 *c, IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint64 wqualfac = iv_magn2q_v3i32(c);
  IVuint8 q2 = q << 1;
  /* Shift +32 bits for Q32.32, if necessary.  */
  IVint8 shift_factor = 0x20 - (IVint8)q2 -
    ((IVint8)soft_fls_i64(iv_magn2q_v3i32(a)) - (IVint8)q2 +
     (IVint8)soft_fls_i64(iv_magn2q_v3i32(b)) - (IVint8)q2);
  if (shift_factor < 0)
    wqualfac >>= -shift_factor;
  else
    wqualfac <<= shift_factor;
  return (IVint32q16)iv_aprx_sqrt_u64(wqualfac);
}

/* Assign the "no solution" value IVINT32_MIN to all components of the
   given vector.  */
IVVec3D_i32 *iv_nosol_v3i32(IVVec3D_i32 *a)
{
  /* Tags: VEC-COMPONENTS */
  a->x = IVINT32_MIN;
  a->y = IVINT32_MIN;
  a->z = IVINT32_MIN;
  return a;
}

/* Test if a vector is equal to the "no solution" vector.  */
IVuint8 iv_is_nosol_v3i32(IVVec3D_i32 *a)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  return (a->x == IVINT32_MIN &&
	  a->y == IVINT32_MIN &&
	  a->z == IVINT32_MIN);
}

/* Test if a vector is equal to zero.  */
IVuint8 iv_is_zero_v3i32(IVVec3D_i32 *a)
{
  return (a->x == 0 && a->y == 0 && a->z == 0);
}

/********************************************************************/
/* Operators that require a square root computation */

IVint32 iv_magnitude_v3i32(IVVec3D_i32 *a)
{
  return iv_sqrt_u64(iv_magn2q_v3i32(a));
}

/* Approximate magnitude, faster but less accurate.  */
IVint32 iv_magn_v3i32(IVVec3D_i32 *a)
{
  return iv_aprx_sqrt_u64(iv_magn2q_v3i32(a));
}

/* Approximate magnitude, faster but less accurate.  The base 2
   logarithm of the result is returned.  */
IVint32 iv_magn_log2_v3i32(IVVec3D_i32 *a)
{
  return iv_aprx_sqrt_log2_u64(iv_magn2q_v3i32(a));
}

/* Doubly approximate vector magnitude determined using find last bit
   set and taking the max of the largest component.  The base 2
   logarithm of the result is returned.  */
IVuint8 iv_amagn_log2_v3i32(IVVec3D_i32 *a)
{
  /* Tags: VEC-COMPONENTS */
  return iv_max3_u8(iv_msbidx_i32(a->x),
		    iv_msbidx_i32(a->y),
		    iv_msbidx_i32(a->z));
}

/* Vector normalization, convert to a Q16.16 fixed-point
   representation.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVNVec3D_i32q16 *iv_normalize2_nv3i32q16_v3i32(IVNVec3D_i32q16 *a,
					       IVVec3D_i32 *b)
{
  IVint32 d = iv_magnitude_v3i32(b);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  return iv_shldiv4_v3i32_i32(a, b, 0x10, d);
}

/* Distance between two points.  */
IVint32 iv_dist2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b)
{
  IVVec3D_i32 t;
  iv_sub3_v3i32(&t, a, b);
  return iv_magnitude_v3i32(&t);
}

/* Approximate distance between two points.  */
IVint32 iv_adist2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b)
{
  IVVec3D_i32 t;
  iv_sub3_v3i32(&t, a, b);
  return iv_magn_v3i32(&t);
}

/* Compute the length of one vector along the direction of another
   vector.

   a_along_b(A, B) = dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVint32 iv_along2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magnitude_v3i32(b);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)(iv_dot2_v3i32(a, b) / d);
}

/* Project a point onto a vector.

   proj_a_on_b(A, B) = B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVVec3D_i32 *iv_proj3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  IVint32 d;
  d = iv_magnitude_v3i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  return iv_muldiv4_v3i32_i64_i32(a, c, iv_dot2_v3i32(b, c), d);
}

/* Eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVVec3D_i32 *iv_elim3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  IVint32 d;
  IVVec3D_i32 t;
  d = iv_magnitude_v3i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64_i32(&t, c, iv_dot2_v3i32(b, c), d);
  iv_sub3_v3i32(a, b, &t);
  return a;
}

/* Approximate eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVVec3D_i32 *iv_aelim3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c)
{
  IVint32 d;
  IVVec3D_i32 t;
  d = iv_magn_v3i32(c);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64_i32(&t, c, iv_dot2_v3i32(b, c), d);
  iv_sub3_v3i32(a, b, &t);
  return a;
}

/* Shortest path distance from point to plane (line in 3D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
IVint32 iv_dist2_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVEqs_v3i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magnitude_v3i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)((iv_dot2_v3i32(a, &b->v) - b->offset) / d);
}

/* Approximate shortest path distance from point to plane (line in 3D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
IVint32 iv_adist2_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVEqs_v3i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d = iv_magn_v3i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (IVint32)((iv_dot2_v3i32(a, &b->v) - b->offset) / d);
}

/* Shortest path distance from point to plane (line in 3D), normalized
   plane surface normal vector, Q16.16 fixed-point.

   Let L = location vector of point,
       A = normalized plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   dot_product(L, A) - d
*/
IVint32q16 iv_dist2_p3i32_Eqs_nv3i32q16(IVPoint3D_i32 *a, IVEqs_nv3i32q16 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  return (IVint32q16)
    (iv_fsyshr2_i64(iv_dot2_v3i32(a, &b->v), 0x10) - b->offset);
}

/* Alternatively, rather than using a scalar `d`, you can define a
   point in the plane as a vector and subtract it from L before
   computing.

   Let L = location vector of point,
       A = plane surface normal vector,
       P = plane location vector

   L_rel_P = L - P;
   dot_product(L_rel_P, A) / magnitude(A);

   If there is no solution, IVINT32_MIN is returned.
*/
IVint32 iv_dist2_p3i32_NRay_v3i32(IVPoint3D_i32 *a, IVNPlane_v3i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d;
  IVVec3D_i32 l_rel_p;
  d = iv_magnitude_v3i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  iv_sub3_v3i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v3i32(&l_rel_p, &b->v) / d);
}

/* Approximate variant of distance to point in plane, like the
   subroutine `iv_dist2_p3i32_NRay_v3i32()`.  */
IVint32 iv_adist2_p3i32_NRay_v3i32(IVPoint3D_i32 *a, IVNPlane_v3i32 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 d;
  IVVec3D_i32 l_rel_p;
  d = iv_magn_v3i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  iv_sub3_v3i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v3i32(&l_rel_p, &b->v) / d);
}

/********************************************************************/

/* Distance squared between two points.  */
IVint64 iv_dist2q2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b)
{
  IVVec3D_i32 t;
  iv_sub3_v3i32(&t, a, b);
  return iv_magn2q_v3i32(&t);
}

/* Project a point to a plane along the perpendicular:

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   L - A * dist_point_to_plane(L, A, d) / magnitude(A)

   Because we are dividing by magnitude twice, the division quantity
   is squared so we therefore can avoid computing the square root and
   simplify as follows:

   L - A * (dot_product(L, A) - d) / magnitude^2(A)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint3D_i32 *iv_proj3_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					IVEqs_v3i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec3D_i32 t;
  IVint64 d;
  d = iv_magn2q_v3i32(&c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64(&t, &c->v, iv_dot2_v3i32(b, &c->v) - c->offset, d);
  return iv_sub3_v3i32(a, b, &t);
}

/* Project a point to a plane along the perpendicular:

   Let L = location vector of point,
       A = plane surface normal vector,
       P = plane location vector

   L - A * dist_point_to_plane(L, P, A) / magnitude(A)

   Because we are dividing by magnitude twice, the division quantity
   is squared so we therefore can avoid computing the square root and
   simplify as follows:

   L_rel_P = L - P;
   L - A * dot_product(L_rel_P, A) / magnitude^2(A);

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint3D_i32 *iv_proj3_p3i32_NPlane_v3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					   IVNPlane_v3i32 *c)
{
  IVVec3D_i32 l_rel_p;
  IVVec3D_i32 t;
  IVint64 d;
  iv_sub3_v3i32(&l_rel_p, b, &c->p0);
  d = iv_magn2q_v3i32(&c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64(&t, &c->v, iv_dot2_v3i32(&l_rel_p, &c->v), d);
  return iv_sub3_v3i32(a, b, &t);
}

/* Bi-directional intersection of line with plane:

   Let D = line direction vector,
       L = line location vector,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   Fully simplified equation:

   L - D * (dot_product(L, A) - d) / dot_product(D, A)

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   NOTE: Often times it is useful to compute `dot_product(D, A)` in
   advance since this can be used as a "quality factor" to check if
   the angle of intersection is well-formed.  Because of this, there a
   separate solver subroutine to handle a precomputed `dot_product(D,
   A)`, to avoid redundantly computing it again.
*/
IVPoint3D_i32 *iv_isect3_InLine_Eqs_v3i32(IVPoint3D_i32 *a,
					  IVInLine_v3i32 *b,
					  IVEqs_v3i32 *c)
{
  IVint64 d;
  d = iv_dot2_v3i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  return iv_p2isect4_InLine_Eqs_v3i32(a, b, c, d);
}

/* Second part of `iv_p2isect3_InLine_Eqs_v3i32()`.  `d` is
   `dot_product(D, A)` and it must not be zero.  */
IVPoint3D_i32 *iv_p2isect4_InLine_Eqs_v3i32(IVPoint3D_i32 *a,
					    IVInLine_v3i32 *b,
					    IVEqs_v3i32 *c,
					    IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec3D_i32 t;
  iv_muldiv4_v3i32_i64(&t, &b->v,
		       iv_dot2_v3i32(&b->p0, &c->v) - c->offset, d);
  return iv_sub3_v3i32(a, &b->p0, &t);
}

/* Bi-directional intersection of line with plane:

   Let D = line direction vector,
       L = line location vector,
       A = plane surface normal vector,
       P = plane location vector

   Fully simplified equations:

   L_rel_P = L - P;
   result = L - D * dot_product(L_rel_P, A) / dot_product(D, A);

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.

   NOTE: Often times it is useful to compute `dot_product(D, A)` in
   advance since this can be used as a "quality factor" to check if
   the angle of intersection is well-formed.  Because of this, there a
   separate solver subroutine to handle a precomputed `dot_product(D,
   A)`, to avoid redundantly computing it again.
*/
IVPoint3D_i32 *iv_isect3_InLine_NPlane_v3i32(IVPoint3D_i32 *a,
					     IVInLine_v3i32 *b,
					     IVNPlane_v3i32 *c)
{
  IVint64 d;
  d = iv_dot2_v3i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  return iv_p2isect4_InLine_NPlane_v3i32(a, b, c, d);
}

/* Second part of `iv_isect3_InLine_NPlane_v3i32()`.  `d` is
   `dot_product(D, A)` and it must not be zero.  */
IVPoint3D_i32 *iv_p2isect4_InLine_NPlane_v3i32(IVPoint3D_i32 *a,
					       IVInLine_v3i32 *b,
					       IVNPlane_v3i32 *c,
					       IVint64 d)
{
  IVVec3D_i32 l_rel_p;
  IVVec3D_i32 t;
  iv_sub3_v3i32(&l_rel_p, &b->p0, &c->p0);
  iv_muldiv4_v3i32_i64(&t, &b->v, iv_dot2_v3i32(&l_rel_p, &c->v), d);
  return iv_sub3_v3i32(a, &b->p0, &t);
}

/* Directional intersection of ray with plane:

   Let D = ray direction vector,
       L = ray location vector,
       A = plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   Fully simplified equation:

   numer = -(dot_product(L, A) - d);
   denom = dot_product(D, A);
   if (numer / denom < 0) then no solution;
   L + D * numer / denom;

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint3D_i32 *iv_isect3_Ray_Eqs_v3i32(IVPoint3D_i32 *a, IVRay_v3i32 *b,
				       IVEqs_v3i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec3D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v3i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  n = -iv_dot2_v3i32(&b->p0, &c->v) + c->offset;
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64(&t, &b->v, n, d);
  return iv_add3_v3i32(a, &b->p0, &t);
}

/* Directional intersection of ray with plane:

   Let D = ray direction vector,
       L = ray location vector,
       A = plane surface normal vector,
       P = plane location vector

   Fully simplified equations:

   L_rel_P = L - P;
   numer = -dot_product(L_rel_P, A);
   denom = dot_product(D, A);
   if (numer / denom < 0) then no solution;
   result = L + D * numer / denom;

   Parameters:
   a = resulting point
   b = ray
   c = plane

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVPoint3D_i32 *iv_isect3_Ray_NPlane_v3i32(IVPoint3D_i32 *a, IVRay_v3i32 *b,
					  IVNPlane_v3i32 *c)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVVec3D_i32 l_rel_p;
  IVVec3D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v3i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_sub3_v3i32(&l_rel_p, &b->p0, &c->p0);
  n = -iv_dot2_v3i32(&l_rel_p, &c->v);
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  iv_muldiv4_v3i32_i64(&t, &b->v, n, d);
  return iv_add3_v3i32(a, &b->p0, &t);
}

/* Quality factor check after computing a dot product, and typically
   before computibng a line-plane intersection.  This is computed as
   follows:

   Let A = line direction vector,
       B = plane surface normal vector,
       D = dot_product(A, B)

   D / (magnitude(A) * magnitude(B))

   If either vector magnitude is zero, the returned quality factor is
   zero.

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated square roots, sometimes the result is greater
   than 1.0 in Q16.16.  */
IVint32q16 iv_postqualfac_dot3_i32q16_v3i32(IVVec3D_i32 *a,
					    IVVec3D_i32 *b,
					    IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32 magn_a, magn_b;
  IVint32q16 norm_d;
  magn_a = iv_magnitude_v3i32(a);
  if (magn_a == 0)
    return 0;
  magn_b = iv_magnitude_v3i32(b);
  if (magn_b == 0)
    return 0;
  if (d >= 0x80000000LL) {
    /* Okay, this is tricky since we don't have much headroom due to
       the 64-bit numerator.  Divide by the smallest factor and the
       remaining factor is guaranteed to be within the headroom of
       IVint32.  So then we can do a standard multiply-divide to
       obtain the fixed-point result.  */
    if (magn_b < magn_a) {
      d /= magn_b;
      norm_d = (IVint32q16)((d << 0x10) / magn_a);
    } else {
      d /= magn_a;
      norm_d = (IVint32q16)((d << 0x10) / magn_b);
    }
  } else {
    IVint64 magn_a_b = (IVint64)magn_a * magn_b;
    norm_d = (IVint32q16)((d << 0x10) / magn_a_b);
  }
  return norm_d;
}

/* Approximate quality factor check after computing a dot product, and
   typically before computibng a line-plane intersection.  This is
   computed as follows:

   Let A = line direction vector,
       B = plane surface normal vector,
       D = dot_product(A, B)

   D / (magnitude(A) * magnitude(B))

   If either vector magnitude is zero, the returned quality factor is
   zero.

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated approximate square roots, sometimes the
   result is greater than 1.0 in Q16.16, often times up to a factor of
   3.0.

   To be fast, use approximate magnitude instead.  Operating directly
   on the results of find last bit set is fastest since you can add
   rather than multiply.  */
IVint32q16 iv_apostqualfac_dot3_i32q16_v3i32(IVVec3D_i32 *a,
					     IVVec3D_i32 *b,
					     IVint64 d)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint64 magn2q_a, magn2q_b;
  IVint8 shift_factor;
  magn2q_a = iv_magn2q_v3i32(a);
  if (magn2q_a == 0)
    return 0;
  magn2q_b = iv_magn2q_v3i32(b);
  if (magn2q_b == 0)
    return 0;
  shift_factor = 0x10 - (((IVint8)soft_fls_i64(magn2q_a) +
			  (IVint8)soft_fls_i64(magn2q_b))
			 >> 1);
  if (shift_factor < 0)
    return (IVint32q16)(d >> -shift_factor);
  /* else */
  return (IVint32q16)(d << shift_factor);
}

/********************************************************************/

/* Convert (reformat) the scalar origin offset of an Eqs
   representational form to point vector representational form.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVVec3D_i32 *iv_rf2_v3i32_Eqs_v3i32(IVVec3D_i32 *a, IVEqs_v3i32 *b)
{
  IVint64 d = iv_magn2q_v3i32(&b->v);
  if (d == 0) {
    /* No solution.  */
    return iv_nosol_v3i32(a);
  }
  return iv_muldiv4_v3i32_i64(a, &b->v, b->offset, d);
}

/* Convert (reformat) Eqs representational form to NPlane.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVNPlane_v3i32 *iv_rf2_NPlane_Eqs_v3i32(IVNPlane_v3i32 *a, IVEqs_v3i32 *b)
{
  /* Convert the origin offset to a point.  */
  iv_rf2_v3i32_Eqs_v3i32(&a->p0, b);
  if (iv_is_nosol_v3i32(&a->p0)) {
    /* No solution.  */
    iv_nosol_v3i32(&a->v);
    return a;
  }
  /* Copy the perpendicular vector.  */
  a->v = b->v;
  return a;
}

/* Convert (reformat) NPlane representational form to Eqs.  */
IVEqs_v3i32 *iv_rf2_Eqs_NPlane_v3i32(IVEqs_v3i32 *a, IVNPlane_v3i32 *b)
{
  IVVec3D_i32 t;
  /* Copy the perpendicular vector.  */
  a->v = b->v;
  /* Essentially, computing the origin offset is computing the
     distance from the origin point to the plane, and then multiplying
     by the length of the surface normal vector.  Therefore, we
     happily avoid computing a square root in a magnitude computation.

     L = 0;
     L_rel_P = L - P;
     dot_product(L_rel_P, A) / magnitude(A)
     = dot_product(-P, A) / magnitude(A);

     Multiply by magnitude(A):
     dot_product(-P, A)
  */
  iv_neg2_v3i32(&t, &b->p0);
  a->offset = iv_dot2_v3i32(&t, &b->v);
  return a;
}

/* Convert (reformat) InPlane representational form to NPlane.  */
IVNPlane_v3i32 *iv_rf2_NPlane_InPlane_v3i32(IVNPlane_v3i32 *a,
					    IVInPlane_v3i32 *b)
{
  /* Copy the location.  */
  a->p0 = b->p0;
  /* Compute the perpendicular vector to use for the NPlane.  */
  iv_crossprod3_cn_v3i32(&a->v, &b->v0, &b->v1);
  return a;
}

/* Convert (reformat) NPlane representational form to InPlane, by
   projecting two hint vectors to determine the two InPlane vectors to
   use.  It is up to you to determine hint vectors that will produce a
   well-formed result.  */
IVInPlane_v3i32 *iv_rfh4_InPlane_NPlane_v3i32
  (IVInPlane_v3i32 *a, IVNPlane_v3i32 *b, IVVec3D_i32 *h0, IVVec3D_i32 *h1)
{
  IVEqs_v3i32 pp; /* projection plane */
  pp.v = b->v; pp.offset = 0;

  /* Copy the location.  */
  a->p0 = b->p0;

  /* Project the two hint vectors to form the InPlane vectors.  */
  iv_proj3_p3i32_Eqs_v3i32(&a->v0, h0, &pp);
  iv_proj3_p3i32_Eqs_v3i32(&a->v1, h1, &pp);
  return a;
}

/* Convert (reformat) NPlane representational form to InPlane, by
   projecting and picking the best of the two given hint vectors and
   using the cross product to compute the other InPlane vector.  If
   the two hint vectors themselves are perpendicular to each other,
   you are guaranteed to always get a good result.  */
IVInPlane_v3i32 *iv_rfbh4_InPlane_NPlane_v3i32
  (IVInPlane_v3i32 *a, IVNPlane_v3i32 *b, IVVec3D_i32 *h0, IVVec3D_i32 *h1)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVuint8 q; /* shift factor */
  IVEqs_v3i32 pp; /* projection plane */
  IVVec3D_i32 h0_v0, h0_v1, h1_v0, h1_v1;
  IVint32 h0_qualfac;
  IVint32 h1_qualfac;
  pp.v = b->v; pp.offset = 0;

  /* Copy the location.  */
  a->p0 = b->p0;

  /* Try using the first hint vector `h0`.  */
  iv_proj3_p3i32_Eqs_v3i32(&a->v0, h0, &pp);
  q = iv_cnf_crossprod2_v3i32(&b->v, &a->v0);
  iv_crossprodshr4_v3i32(&a->v1, &b->v, &a->v0, q);
  h0_v0 = a->v0;
  h0_v1 = a->v1;

  /* Check the quality of the vector cross product using fast
     approximate math.  */
  h0_qualfac = iv_apostqualfac_cps4_i32q16_v3i32(&a->v1, &b->v, &a->v0, q);

  /* N.B.: Less than 1/8 is actually pretty bad quality but we can
     still get reasonable results.  We set our threshold even lower so
     that we will be sure to reject only extremely poor quality
     choices, so that the other choice is pretty much guaranteed to
     have sufficient quality.  Alternatively, computing both quality
     metrics would guarantee more robust choices.  */
  if (h0_qualfac >= 0x0100) { /* 1/256 in Q16.16 */
    /* Sufficient quality, return this result.  */
    /* return a; */
  }

  /* Since the first hint was of insufficient quality, we must use the
     second hint instead.  Don't bother computing the quality factor
     on this one since it should be unnecessary if the user is using
     this subroutine correctly.  */
  iv_proj3_p3i32_Eqs_v3i32(&a->v0, h1, &pp);
  q = iv_cnf_crossprod2_v3i32(&b->v, &a->v0);
  iv_crossprodshr4_v3i32(&a->v1, &b->v, &a->v0, q);
  h1_v0 = a->v0;
  h1_v1 = a->v1;

  h1_qualfac = iv_apostqualfac_cps4_i32q16_v3i32(&a->v1, &b->v, &a->v0, q);

  if (h0_qualfac >= h1_qualfac) {
    /* Use the first hint.  */
    a->v0 = h0_v0;
    a->v1 = h0_v1;
  } else {
    /* Use the second hint.  */
    a->v0 = h1_v0;
    a->v1 = h1_v1;
  }

  return a;
}

/* Solve a system of three simple linear equations, i.e. Ax = b format.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint3D_i32 *iv_solve2_s3_Eqs_v3i32(IVPoint3D_i32 *a, IVSys3_Eqs_v3i32 *b)
{
  IVEqs_v3i32 pp;  /* projection plane */
  IVInLine_v3i32 ipv0; /* one InPlane line of plane 0 */
  IVInLine_v3i32 e0e1; /* intersection between plane 0 and plane 1 */
  /* Pre-check, reject with no solution if any vector is zero.  */
  if (iv_is_zero_v3i32(&b->d[0].v) ||
      iv_is_zero_v3i32(&b->d[1].v) ||
      iv_is_zero_v3i32(&b->d[2].v)) {
    /* No solution.  */
    iv_nosol_v3i32(a);
    return a;
  }
  /* 1. Project plane 1's surface normal vector onto plane 0's so that
     we can head straight toward plane 1 from that generated InPlane
     vector.  */
  pp.v = b->d[0].v; pp.offset = 0;
  iv_proj3_p3i32_Eqs_v3i32(&ipv0.v, &b->d[1].v, &pp);
  /* 2. Compute the intersection point between plane 0 and plane 1.  */
  iv_rf2_v3i32_Eqs_v3i32(&ipv0.p0, &b->d[0]);
  iv_isect3_InLine_Eqs_v3i32(&e0e1.p0, &ipv0, &b->d[1]);
  if (iv_is_nosol_v3i32(&e0e1.p0)) {
    /* No solution.  */
    iv_nosol_v3i32(a);
    return a;
  }
  /* 3. Compute the cross product of plane 0 and plane 1's surface
     normal vectors.  */
  iv_crossprod3_cn_v3i32(&e0e1.v, &b->d[0].v, &b->d[1].v);
  /* 4. Using the defined ray of the intersection between plane 0 and
        1, compute the line-plane intersection with the final plane to
        determine the final solution.  */
  return iv_isect3_InLine_Eqs_v3i32(a, &e0e1, &b->d[2]);
}

/* Solve a system of three "surface-normal" perpendicular vector
   linear equations.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint3D_i32 *iv_solve2_s3_NPlane_v3i32(IVPoint3D_i32 *a,
					 IVSys3_NPlane_v3i32 *b)
{
  IVEqs_v3i32 pp;  /* projection plane */
  IVInLine_v3i32 ipv0; /* one InPlane line of plane 0 */
  IVInLine_v3i32 e0e1; /* intersection between plane 0 and plane 1 */
  /* Pre-check, reject with no solution if any vector is zero.  */
  if (iv_is_zero_v3i32(&b->d[0].v) ||
      iv_is_zero_v3i32(&b->d[1].v) ||
      iv_is_zero_v3i32(&b->d[2].v)) {
    /* No solution.  */
    iv_nosol_v3i32(a);
    return a;
  }
  /* 1. Project plane 1's surface normal vector onto plane 0's so that
     we can head straight toward plane 1 from that generated InPlane
     vector.  */
  /* NOTE ALTERNATE STRATEGY: Rather than using cross products to get
     surface normal vectors, we could project points.  How???  */
  pp.v = b->d[0].v; pp.offset = 0;
  iv_proj3_p3i32_Eqs_v3i32(&ipv0.v, &b->d[1].v, &pp);
  /* 2. Compute the intersection point between plane 0 and plane 1.  */
  ipv0.p0 = b->d[0].p0;
  iv_isect3_InLine_NPlane_v3i32(&e0e1.p0, &ipv0, &b->d[1]);
  if (iv_is_nosol_v3i32(&e0e1.p0)) {
    /* No solution.  */
    iv_nosol_v3i32(a);
    return a;
  }
  /* 3. Compute the cross product of plane 0 and plane 1's surface
     normal vectors.  */
  iv_crossprod3_cn_v3i32(&e0e1.v, &b->d[0].v, &b->d[1].v);
  /* 4. Using the defined ray of the intersection between plane 0 and
        1, compute the line-plane intersection with the final plane to
        determine the final solution.  */
  return iv_isect3_InLine_NPlane_v3i32(a, &e0e1, &b->d[2]);
}

/* Solve a system of three InPlane vector linear equations.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint3D_i32 *iv_solve2_s3_InPlane_v3i32(IVPoint3D_i32 *a,
					  IVSys3_InPlane_v3i32 *b)
{
  /* 1. Compute the quality factors on the dot products of each
     InPlane vector with the ...  */
  /* Okay, so how do we solve intersections with InPlanes?  Here's the
     trick.  First you treat each InPlane vector as an NPlane of its
     own.  Compute the distance from your point to each NPlane.  Now
     you can switch back to the default interpretation.  Navigate on
     the InPlane using the lengths you found along the vectors.  Your
     destination will be the projected point position on the plane.
     Finally, with this knowledge, you can compute this distance and
     use it to solve for an ray intersection with the InPlane.

     Okay, so thinking this through a second time, there is a more
     efficient way.  At the first step, you can project the point onto
     the corresponding NPlanes.  And the other thing, do the
     projection one step at a time rather than trying to do both at
     the same time, else you'll double-count.  Now, you need to
     subtract to determine the displacement vectors, rather than
     multiplying by the distances.  Add these, and you've got the
     point projected on the plane.  So effectively... you're trying to
     generate your own pristine surface normal staying closer to the
     data than would be the case if you were to reformat into the
     NPlane representational form.

     Or hey, here's what you're doing in the early stages.  After
     you're done projecting with the two NPlanes, you've determined a
     point elevated above the origin point in the plane.  Subtract to
     get the displacement above the plane, then you can compute your
     custom normal.  */

  /* PLEASE NOTE: Important optimization!  Projecting a point to an
     InPlane representational form does not require any division
     computations!  It can therefore be faster and more efficient,
     under certain circumstances.  Yes, even with non-normalized
     vectors.  Otherwise, of course, the normalized vector form uses
     shifts in place of divisions.  Since you have to divide to
     normalize, sometimes normalization doesn't buy you anything in
     efficiency.

     Wait, hold on.  Dot product method?  Don'y you have to divide
     there too to compute the actual point location?  Oh, yeah, unless
     you have normalized vectors.  Worse, you also have to compute the
     square root of the length.  */
}

/* Quality factor check on system of 3 equations before solving.  This
   is computed using the cross product of the first two surface normal
   vectors times the absolute value of the dot product between that
   and the final surface normal vector.

   The result is in Q16.16 fixed-point format.  Note that since we're
   dividing by truncated square roots, sometimes the result is greater
   than 1.0 in Q16.16.  */
IVint32q16 iv_prequalfac_i32q16_s3_Eqs_v3i32(IVSys3_Eqs_v3i32 *a)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32q16 qualfac1, qualfac2;
  IVuint8 q;
  IVVec3D_i32 v0_cross_v1;
  IVint64 v0v1_dot_v2;
  q = iv_cnf_crossprod2_v3i32(&a->d[0].v, &a->d[1].v);
  iv_crossprodshr4_v3i32(&v0_cross_v1, &a->d[0].v, &a->d[1].v, q);
  qualfac1 = iv_postqualfac_cps4_i32q16_v3i32(&v0_cross_v1,
					      &a->d[0].v, &a->d[1].v, q);
  v0v1_dot_v2 = iv_abs_i64(iv_dot2_v3i32(&v0_cross_v1, &a->d[2].v));
  qualfac2 = iv_postqualfac_dot3_i32q16_v3i32(&v0_cross_v1, &a->d[2].v,
					      v0v1_dot_v2);
  return (IVint32q16)(((IVint64)qualfac1 * qualfac2) >> 0x10);  
}

/* Approximate quality factor check on system of 3 equations before
   solving.  This is computed using the cross product of the first two
   surface normal vectors times the absolute value of the dot product
   between that and the final surface normal vector.  */
IVint32q16 iv_aprequalfac_i32q16_s3_Eqs_v3i32(IVSys3_Eqs_v3i32 *a)
{
  /* Tags: SCALAR-ARITHMETIC */
  IVint32q16 qualfac1, qualfac2;
  IVuint8 q;
  IVVec3D_i32 v0_cross_v1;
  IVint64 v0v1_dot_v2;
  q = iv_cnf_crossprod2_v3i32(&a->d[0].v, &a->d[1].v);
  iv_crossprodshr4_v3i32(&v0_cross_v1, &a->d[0].v, &a->d[1].v, q);
  qualfac1 = iv_apostqualfac_cps4_i32q16_v3i32(&v0_cross_v1,
					       &a->d[0].v, &a->d[1].v, q);
  v0v1_dot_v2 = iv_abs_i64(iv_dot2_v3i32(&v0_cross_v1, &a->d[2].v));
  qualfac2 = iv_apostqualfac_dot3_i32q16_v3i32(&v0_cross_v1, &a->d[2].v,
					       v0v1_dot_v2);
  return (IVint32q16)(((IVint64)qualfac1 * qualfac2) >> 0x10);  
}

/********************************************************************/
/* Statistical routines */

/* Compute the mean point coordinate of a set of points.  Division
   is performed at the very end as follows:

   sum(i = 1 to n, x_i) / n

   The intermediate sum computation uses the same precision as the
   final result.  */
IVVec3D_i32 *iv_mean2_v3i32_arn_v3i32(IVPoint3D_i32 *a,
				      IVPoint3D_i32_array *data)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVPoint3D_i32 *data_d = data->d;
  IVPoint3D_i32 w = { 0, 0 };
  IVuint16 data_len = data->len;
  IVuint16 i;

  for (i = 0; i < data_len; i++) {
    w.x += data_d[i].x;
    w.y += data_d[i].y;
    w.z += data_d[i].z;
  }
  w.x /= data_len;
  w.y /= data_len;
  w.z /= data_len;

  *a = w;
  return a;
}

/* Set up system of equations to compute a linear regression of the "y
   = m*x + b" representational form.  When the system is solved, the
   result vector format is `[b, m]`.  The equations are formatted for
   solving in Q16.16 fixed-point format since you'll need a
   fixed-point number for the fractional slope.

   `q` = shift right factor, bits after decimal for fixed-point
   operands

   If there is a memory allocation error, the system of equation's
   entries are all set to IVINT32_MIN (and IVINT64_MIN).  */
IVSys3_Eqs_v3i32q16 *iv_pack_linreg3_s3_Eqs_v3i32q16_arn_v3i32
  (IVSys3_Eqs_v3i32q16 *sys, IVPoint2D_i32_array *data, IVuint8 q)
{
  IVPoint2D_i32 *data_d = data->d;
  IVuint16 data_len = data->len;
  IVMatNxM_i32 mat_a; /* Matrix A */
  IVMatNxM_i32 mat_a_t; /* Matrix A^T */
  IVMatNxM_i32 col_b; /* Column vector b */
  IVMatNxM_i32 mat_a_t_a; /* Matrix A^T * A */
  IVMat2x2_i32 mat_a_t_a_stor; /* Allocation for Matrix A^T * A */
  IVMatNxM_i32 col_a_t_b; /* Column vector A^T * b */
  IVVec2D_i32 col_a_t_b_stor; /* Allocation for Column vector A^T * b */
  IVuint16 i;

  /* Pre-allocate all intermediate data structures.  */
  mat_a.d = NULL;
  mat_a_t.d = NULL;
  col_b.d = NULL;

  mat_a.width = 3;
  mat_a.height = data_len;
  mat_a.d = (IVint32*)malloc(sizeof(IVint32) * mat_a.width * mat_a.height);
  if (mat_a.d == NULL)
    goto dirty_cleanup;

  mat_a_t.width = mat_a.height;
  mat_a_t.height = mat_a.width;
  mat_a_t.d =
    (IVint32*)malloc(sizeof(IVint32) * mat_a_t.width * mat_a_t.height);
  if (mat_a_t.d == NULL)
    goto dirty_cleanup;

  col_b.width = 1;
  col_b.height = data_len;
  col_b.d =
    (IVint32*)malloc(sizeof(IVint32) * col_b.width * col_b.height);
  if (col_b.d == NULL)
    goto dirty_cleanup;

  mat_a_t_a.width = 3;
  mat_a_t_a.height = 3;
  mat_a_t_a.d = mat_a_t_a_stor.d;
  col_a_t_b.width = 1;
  col_a_t_b.height = 3;
  col_a_t_b.d = (IVint32*)&col_a_t_b_stor;

  /* Pack into the coefficient matrix `A`.  */
  for (i = 0; i < data_len; i++) {
    mat_a.d[mat_a.width*i+0] = 1 << q;
    mat_a.d[mat_a.width*i+1] = data_d[i].x;
    mat_a.d[mat_a.width*i+2] = data_d[i].x * data_d[i].x;
  }
  /* Compute `A^T`.  */
  iv_xpose2_mnxm_i32(&mat_a_t, &mat_a);

  /* Pack into the coefficient matrix `b`.  */
  for (i = 0; i < data_len; i++) {
    col_b.d[i] = data_d[i].y;
  }

  /* Compute `A^T * A`.  */
  iv_mulshr4_mnxm_i32(&mat_a_t_a, &mat_a_t, &mat_a, q);

  /* Compute `A^T * b`.  */
  iv_mulshr4_mnxm_i32(&col_a_t_b, &mat_a_t, &col_b, q);

  /* Pack into IVSys3_Eqs_v3i32 representational form.  */
  sys->d[0].v.x = mat_a_t_a.d[0];
  sys->d[0].v.y = mat_a_t_a.d[1];
  sys->d[0].v.z = mat_a_t_a.d[2];
  sys->d[0].offset = col_a_t_b.d[0];
  sys->d[1].v.x = mat_a_t_a.d[3];
  sys->d[1].v.y = mat_a_t_a.d[4];
  sys->d[1].v.z = mat_a_t_a.d[5];
  sys->d[1].offset = col_a_t_b.d[1];
  sys->d[2].v.x = mat_a_t_a.d[6];
  sys->d[2].v.y = mat_a_t_a.d[7];
  sys->d[2].v.z = mat_a_t_a.d[8];
  sys->d[2].offset = col_a_t_b.d[2];

  /* Here's what we're doing here.  Since we'll need a fixed-point
     solution, our equation solver will operate with a Q16.16 decimal
     point.  We use the "cheap normalization" technique.  For large
     integers, they can just be passed in without modification and it
     will all work, since in a system of equations you can divide all
     entries by the same number and get the same result.  However, if
     our inputs are small integers and we input them as-is, some
     values may get shifted to zero in the computations, so we
     adaptively shift all entries right based off of an entry that is
     similar to the median value of the entries.  */
  {
    IVint32 test_value = sys->d[0].v.z;
    IVuint8 shr_div = iv_msbidx_i32(test_value);
    IVuint8 num_shr;
    if (shr_div > 0x10)
      shr_div = 0x10;
    num_shr = 0x10 - shr_div;
    sys->d[0].v.x <<= num_shr;
    sys->d[0].v.y <<= num_shr;
    sys->d[0].v.z <<= num_shr;
    sys->d[1].v.x <<= num_shr;
    sys->d[1].v.y <<= num_shr;
    sys->d[1].v.z <<= num_shr;
    sys->d[2].v.x <<= num_shr;
    sys->d[2].v.y <<= num_shr;
    sys->d[2].v.z <<= num_shr;
    /* `offset` must be shifted twice as many bits.  */
    num_shr = 0x20 - shr_div;
    sys->d[0].offset <<= num_shr;
    sys->d[1].offset <<= num_shr;
    sys->d[2].offset <<= num_shr;
  }

  /* Cleanup.  */
  free (mat_a.d);
  free (mat_a_t.d);
  free (col_b.d);

  return sys;

 dirty_cleanup:
  iv_nosol_v3i32(&sys->d[0].v);
  sys->d[0].offset = IVINT64_MIN;
  iv_nosol_v3i32(&sys->d[1].v);
  sys->d[1].offset = IVINT64_MIN;
  iv_nosol_v3i32(&sys->d[2].v);
  sys->d[2].offset = IVINT64_MIN;
  if (mat_a.d != NULL) free (mat_a.d);
  if (mat_a_t.d != NULL) free (mat_a_t.d);
  if (col_b.d != NULL) free (col_b.d);
  return sys;
}

/********************************************************************/
/* Surface area and volume routines */

/* Compute the area of a 2D triangle using the determinant of two
   non-normalized edge vectors.

   abs(det([A, B])) / 2 = abs(A_x * B_y - B_x * A_y) / 2

   `q` = shift right factor, bits after decimal for fixed-point
   operands
*/
IVuint64 iv_triarea2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* Add one to the symmetric shift right factor to also divide by two
     at the same time.  */
  return iv_abs_i64(iv_fsyshr2_i64((IVint64)a->x * b->y -
				   (IVint64)b->x * a->y, q + 1));
}

/* Compute the area of a 3D triangle using the cross product of two
   non-normalized edge vectors.

   magnitude(A) * magnitude(B) * sin(theta) / 2
   = magnitude(cross_product(A, B)) / 2

   `q` = shift right factor, bits after decimal for fixed-point
   operands
*/
IVuint64 iv_triarea2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q)
{
  IVVec3D_i32 t;
  /* TODO FIXME: Numeric overflow control is needed here.  */
  iv_crossprodshr4_v3i32(&t, a, b, q);
  return iv_fsyshr2_i64(iv_magnitude_v3i32(&t), 1);
}

/* Compute the volume of a pyramid.  `a` = area of base, `b` =
   height.  */
IVuint64 iv_pyrvol2_u64_u32(IVuint64 a, IVuint32 b)
{
  /* TODO FIXME: Numeric overflow control is needed here.  */
  return a * b / 3;
}

/* NOTE: To compute the volume of an arbitrary convex polyhedron:

   1. Find some sort of center point.  For ease of computation, just
      take the mean value of all the vertices, all the face centers,
      or similar.

   2. Compute the area of each face.  Do this tessellating each convex
      polygon face into triangles via the "triangle fan" technique for
      simplicity.  Otherwise, you could use the central vertex
      technique again as another way to tessellate.

   3. Compute the perpendicular height from each face to the
      polyhedron center point.

   4. Compute the pyramid volume for each face and add up the result.

   Now if you want to compute the volume of an arbitrary triangle
   mesh, one way is to tessellate it into a collection of convex
   polyhedra, then add up the volumes of each.  But how do you compute
   that tessellation?

   I'll throw out one idea.  Deflate the faces until the center is
   filled.  This helps in the simplification function because often
   times, deflating will result in some faces getting trimmed off
   before the center is reached.  Then for those faces that extend
   beyond the center before trimming into a pyramid, it's merely a
   matter of figuring out where you need to chop off early the top to
   make it into a "frustum"-like solid.  In concave regions, you'll
   have an opening frustum rather than a closing pyramid.

*/

/********************************************************************/
/* 3D rendering transformation routines */

/* TODO FIXME: Important optimizations!  If the world width and height
   is a power of two in the world-to-viewport conversion, then we can
   shift right rather than divide.  Likewise for focus in the
   perspective unproject transformation.  For perspective project,
   however, we must still divide since the z-coordinate is in the
   denominator, but power-of-two gives us the opportunity to shift
   left rather than multiply.  */

/* Convert world coordinates to viewport coordinates using the given
   bounding boxes.

   World rectangle is (xmin, ymin, width, height) format.  Viewport
   rectangle is (x_offset, y_offset, width, height) format.  World
   rectangle width and height must not be equal to zero, or else
   division by zero will occur.  */
IVPoint3D_i32 *iv_w2v4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
			     IVRect_p2i32 *world, IVRect_p2i32 *viewport)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVint32 w_width = world->d[1].x;
  IVint32 w_height = world->d[1].y;
  IVint32 v_width = viewport->d[1].x;
  IVint32 v_height = viewport->d[1].y;
  IVPoint2D_i32 t;

  /* Subtract the (xmin, ymin) coordinate.  */
  t.x = b->x - world->d[0].x;
  t.y = b->y - world->d[0].y;

  /* Scale to viewport coordinates and add viewport offset.  */
  /* TODO FIXME: Define `iv_muldiv3_i32 ()`.  */
  a->x = (IVint32)((IVint64)t.x * v_width / w_width) + viewport->d[0].x;
  a->y = (IVint32)((IVint64)t.y * v_height / w_height) + viewport->d[0].x;

  /* Finally, copy the z coordinate as-is so it can be used to
     generate the z buffer.  */
  a->z = b->z;

  return a;
}

/* Convert world coordinates to viewport coordinates using the given
   bounding boxes and efficient shift right arithmetic.

   World rectangle is (xmin, ymin, width_log2, height_log2) format.
   Viewport rectangle is (x_offset, y_offset, width, height)
   format.  */
IVPoint3D_i32 *iv_w2v_shr4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				 IVRect_p2i32 *world,
				 IVRect_p2i32 *viewport)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVint32 w_width_log2 = world->d[1].x;
  IVint32 w_height_log2 = world->d[1].y;
  IVint32 v_width = viewport->d[1].x;
  IVint32 v_height = viewport->d[1].y;
  IVPoint2D_i32 t;

  /* Subtract the (xmin, ymin) coordinate.  */
  t.x = b->x - world->d[0].x;
  t.y = b->y - world->d[0].y;

  /* Scale to viewport coordinates and add viewport offset.  */
  /* TODO FIXME: Define mulshr for both signed and unsigned
     scalars.  */
  a->x = (IVint32)(((IVint64)t.x * v_width) >> w_width_log2) +
    viewport->d[0].x;
  a->y = (IVint32)(((IVint64)t.y * v_height) >> w_height_log2) +
    viewport->d[0].y;

  /* Finally, copy the z coordinate as-is so it can be used to
     generate the z buffer.  */
  a->z = b->z;

  return a;
}

/* Convert world coordinates to viewport coordinates using the given
   bounding boxes and highly efficient shift right arithmetic.  The
   world rectangle must be larger than the viewport rectangle, which
   will typically be the case with fixed-point world coordinates and
   pixel viewport coordinates.

   World rectangle is (xmin, ymin, width_log2, height_log2) format.
   Viewport rectangle is (x_offset, y_offset, width_log2, height_log2)
   format.  */
IVPoint3D_i32 *iv_w2v_eshr4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				  IVRect_p2i32 *world,
				  IVRect_p2i32 *viewport)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  IVint32 w_width_log2 = world->d[1].x;
  IVint32 w_height_log2 = world->d[1].y;
  IVint32 v_width_log2 = viewport->d[1].x;
  IVint32 v_height_log2 = viewport->d[1].y;
  IVPoint2D_i32 t;

  /* Subtract the (xmin, ymin) coordinate.  */
  t.x = b->x - world->d[0].x;
  t.y = b->y - world->d[0].y;

  /* Scale to viewport coordinates and add viewport offset.  */
  /* TODO FIXME: Define mulshr for both signed and unsigned
     scalars.  */
  a->x = t.x >> (w_width_log2 - v_width_log2) + viewport->d[0].x;
  a->y = t.y >> (w_height_log2 - v_height_log2) + viewport->d[0].y;

  /* Finally, copy the z coordinate as-is so it can be used to
     generate the z buffer.  */
  a->z = b->z;

  return a;
}

/* TODO: Convert from viewport coordinates to world coordinates.  This
   is particularly useful for 3D manipulation via a GUI pointer device
   such as a mouse.  */

/* Perspective project the given coordinate.  The z coordinate is
   passed through as-is.  `z` must not be equal to zero, or else
   division by zero will occur.  */
IVPoint3D_i32 *iv_persproj3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				  IVint32 focus)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* TODO FIXME: Use scalar muldiv.  */
  a->x = (IVint32)((IVint64)b->x * focus / b->z);
  a->y = (IVint32)((IVint64)b->y * focus / b->z);
  a->z = b->z;
  return a;
}

/* Reverse perspective project the given coordinate.  The z
   coordinate is passed through as-is.  This is particularly useful
   for 3D manipulation via a GUI pointer device such as a mouse.
   `focus` must not be equal to zero, or else division by zero will
   occur.  */
IVPoint3D_i32 *iv_unpersproj3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				    IVint32 focus)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* TODO FIXME: Use scalar muldiv.  */
  a->x = (IVint32)((IVint64)b->x * b->z / focus);
  a->y = (IVint32)((IVint64)b->y * b->z / focus);
  a->z = b->z;
  return a;
}

/* Perspective project the given coordinate.  The z coordinate is also
   homogenized so that you can generate a z buffer with non-linear
   precision.  This is only precise if there are a sufficient number
   of bits after the decimal specified.  `z` must not be equal to
   zero, or else division by zero will occur.

   `q` = shift right factor, bits after decimal for fixed-point
   operands
*/
IVPoint3D_i32 *iv_persproj_homo3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				       IVint32 focus, IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* TODO FIXME: Use scalar muldiv.  */
  a->x = (IVint32)((IVint64)b->x * focus / b->z);
  a->y = (IVint32)((IVint64)b->y * focus / b->z);
  /* TODO FIXME: Use scalar shldiv.  */
  a->z = (IVint32)(((IVint64)focus << q) / b->z);
  return a;
}

/* Reverse perspective project the given coordinate.  The z coordinate
   is de-homogenized.  This is only precise if there are a sufficient
   number of bits after the decimal specified.  This is particularly
   useful for 3D manipulation via a GUI pointer device such as a
   mouse.  `focus` and `z` must not be equal to zero, or else division
   by zero will occur.

   `q` = shift right factor, bits after decimal for fixed-point
   operands
*/
IVPoint3D_i32 *iv_unpersproj_homo3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					 IVint32 focus, IVuint8 q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* TODO FIXME: Use scalar shldiv.  */
  a->z = (IVint32)((IVint64)(focus << q) / b->z);
  /* TODO FIXME: Use scalar muldiv.  */
  a->x = (IVint32)((IVint64)b->x * a->z / focus);
  a->y = (IVint32)((IVint64)b->y * a->z / focus);
  return a;
}

/* TODO: Power of two focus -> more efficient perspective
   projection.  */

/* TODO: Translate, rotate, and scale matrices.  2x2 and 3x3 only.  We
   only do affine transformations, non-affine transformations require
   more care with integer arithmetic and are therefore handled
   separately.  And we generate matrices since the same
   transformations need to be performed repeatedly on many points and
   this is most efficient, rather than transformation lists.  */
