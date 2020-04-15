/* Simple integer vector math subroutines.  A second iteration on my
   previous complicated generics design.  We only support 32-bit
   integers with 64-bit intermediates, two dimensions, and three
   dimensions.

   Fixed-point arithmetic is only used as an intermediate form,
   Q32.32.  */

#include <stdio.h>
#include <stdlib.h>

#include "ivecmath.h"

/* TODO FIXME temporary!  */
IVuint8 g_shr = 0;

IVVec2D_i32 *iv_neg2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  a->d[IX] = -b->d[IX];
  a->d[IY] = -b->d[IY];
  return a;
}

IVVec2D_i32 *iv_add3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  a->d[IX] = b->d[IX] + c->d[IX];
  a->d[IY] = b->d[IY] + c->d[IY];
  return a;
}

IVVec2D_i32 *iv_sub3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  a->d[IX] = b->d[IX] - c->d[IX];
  a->d[IY] = b->d[IY] - c->d[IY];
  return a;
}

/* multiply, divide */
IVVec2D_i32 *iv_muldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVint32 d)
{
  a->d[IX] = (IVint32)((IVint64)b->d[IX] * c / d);
  a->d[IY] = (IVint32)((IVint64)b->d[IY] * c / d);
  return a;
}

/* multiply, divide, 64-bit constants, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec2D_i32 *iv_muldiv4_v2i32_i64(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint64 c, IVint64 d)
{
  if (d < 0) { c = -c; d = -d; }
  if (d >= 0x100000000LL) {
    /* Shift away all the low bits from both the numerator and the
       denominator, they are insignificant.  */
    if (c < 0) c++; /* avoid asymmetric two's complement behavior */
    c >>= 0x20;
    d >>= 0x20;
  } else if (c >= 0x100000000LL || c <= -0x100000000LL) {
    /* Fold c / d together before multiplying to try to avoid overflow
       when possible.  */
    c /= d;
    a->d[IX] = (IVint32)((IVint64)b->d[IX] * c);
    a->d[IY] = (IVint32)((IVint64)b->d[IY] * c);
    return a;
  }
  a->d[IX] = (IVint32)((IVint64)b->d[IX] * c / d);
  a->d[IY] = (IVint32)((IVint64)b->d[IY] * c / d);
  return a;
}

/* multiply 64-bit, divide 32-bit, with logic to try to avoid
   overflows when possible while still using 64-bit intermediates */
IVVec2D_i32 *iv_muldiv4_v2i32_i64_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				      IVint64 c, IVint32 d)
{
  if (c >= 0x100000000LL || c <= -0x100000000LL) {
    /* Fold c / d together before multiplying to try to avoid overflow
       when possible.  */
    c /= d;
    a->d[IX] = (IVint32)((IVint64)b->d[IX] * c);
    a->d[IY] = (IVint32)((IVint64)b->d[IY] * c);
    return a;
  }
  a->d[IX] = (IVint32)((IVint64)b->d[IX] * c / d);
  a->d[IY] = (IVint32)((IVint64)b->d[IY] * c / d);
  return a;
}

/* shift left, divide */
IVVec2D_i32 *iv_shldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVuint32 c, IVint32 d)
{
  a->d[IX] = (IVint32)((IVint64)b->d[IX] << c / d);
  a->d[IY] = (IVint32)((IVint64)b->d[IY] << c / d);
  return a;
}

/* multiply, shift right, with symmetric positive/negative shift
   behavior */
IVVec2D_i32 *iv_mulshr4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVuint32 d)
{
  IVint64 tx = b->d[IX] * c;
  IVint64 ty = b->d[IY] * c;
  /* Due to asymmetric two's complement, we must pre-increment one if
     less than zero.  */
  if (d != 0) {
    if (tx < 0) tx++;
    if (ty < 0) ty++;
  }
  a->d[IX] = (IVint32)(tx >> d);
  a->d[IY] = (IVint32)(ty >> d);
  return a;
}

/* shift left */
IVVec2D_i32 *iv_shl3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint32 c)
{
  a->d[IX] = b->d[IX] << c;
  a->d[IY] = b->d[IY] << c;
  return a;
}

/* shift right, with symmetric positive/negative shift behavior */
IVVec2D_i32 *iv_shr3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint32 c)
{
  IVint32 tx = b->d[IX], ty = b->d[IY];
  if (c == 0) {
    a->d[IX] = tx;
    a->d[IY] = ty;
    return a;
  }
  /* Due to asymmetric two's complement, we must pre-increment one if
     less than zero.  */
  if (tx < 0) tx++;
  if (ty < 0) ty++;
  a->d[IX] = tx >> c;
  a->d[IY] = ty >> c;
  return a;
}

IVint64 iv_dot2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  return (IVint64)a->d[IX] * b->d[IX] + (IVint64)a->d[IY] * b->d[IY];
}

/********************************************************************/

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
  a->d[IX] = (IVint32)(((IVint64)b->d[IY] * c->d[IZ] -
			(IVint64)b->d[IZ] * c->d[IY]) / d);
  a->d[IY] = (IVint32)(((IVint64)b->d[IZ] * c->d[IX] -
			(IVint64)b->d[IX] * c->d[IZ]) / d);
  a->d[IZ] = (IVint32)(((IVint64)b->d[IX] * c->d[IY] -
			(IVint64)b->d[IY] * c->d[IX]) / d);
  return a;
}

IVVec3D_i32 *iv_crossprodshr4_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c,
				    IVuint32 d)
{
  /* TODO FIXME SYMMETRIC SHIFT BEHAVIOR */
  /* TODO VERIFY PRECISION */
  a->d[IX] = (IVint32)(((IVint64)b->d[IY] * c->d[IZ] -
			(IVint64)b->d[IZ] * c->d[IY]) >> d);
  a->d[IY] = (IVint32)(((IVint64)b->d[IZ] * c->d[IX] -
			(IVint64)b->d[IX] * c->d[IZ]) >> d);
  a->d[IZ] = (IVint32)(((IVint64)b->d[IX] * c->d[IY] -
			(IVint64)b->d[IY] * c->d[IX]) >> d);
  return a;
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
  using the bit-scan reverse CPU instruction (`bsr` in x86 CPUs) and
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
  is initialized using the bit-scan reverse method to determine the
  approximate square root.

All methods are designed to truncate the result, as is standard with
integer arithmetic, when there is no exact square root solution
available.

*/

static IVuint32 *sqrt_lut = NULL;

/* Fallback implementation of bit-scan reverse in software.  */
IVuint8 soft_bsr_i64(IVint64 a)
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
     three.  Also, we use != comparison since shift_idx may go
     negative and it's unsigned.  */
  shift = pos - 3;
  while (pos != shift) {
    if ((a & mask))
      return pos;
    mask >>= 1;
    pos--;
  }
  return pos;
}

/* Fallback implementation of bit-scan reverse in software, but
   doesn't use any shifting instructions at all!  This is useful if
   your CPU does not have bit-shifting instructions (Gigatron), or it
   doesn't support multi-bit shifts (i.e. 6502).  */
IVuint8 soft_ns_bsr_i64(IVint64 a)
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
     three.  Also, we use != comparison since shift_idx may go
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

/* Compute a 64-bit integer approximate square root by using bit-scan
   reverse and dividing the number of significant bits by two.  Yes,
   we shift right to divide by two.  If the operand is negative,
   IVINT32_MIN is returned since there is no solution.  */
IVint32 iv_aprx_sqrt_i64(IVint64 a)
{
  IVuint8 num_sig_bits;
  if (a == 0)
    return 0;
  if (a < 0)
    return IVINT32_MIN;
  num_sig_bits = soft_bsr_i64(a);
  return (IVint32)1 << (num_sig_bits >> 1);
}

/* This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
IVint32 iv_sqrt_i64(IVint64 a)
{
  IVuint8 pos;
  IVint32 x;
  IVint64 x2;
  if (a == 0)
    return 0;
  if (a < 0)
    return IVINT32_MIN;
  if (a <= 0xffffffff && sqrt_lut != NULL) {
    return iv_sqrt_u32((IVuint32)a);
  }
  pos = soft_bsr_i64(a) >> 1;
  x = (IVint32)1 << pos;
  x2 = (IVint64)1 << (pos << 1);
  pos--;
  while (pos != (IVuint8)-1) {
    IVint32 n = (IVint32)1 << pos; /* n == 2^pos */
    IVint64 n2 = (IVint64)1 << (pos << 1); /* n^2 */
    IVint64 xn2 = (IVint64)x << (pos + 1); /* 2*x*n */
    /* (x + n)^2 = x^2 + 2*x*n + n^2 */
    IVint64 test = x2 + xn2 + n2;
    if (test <= a) {
      x += n;
      x2 = test;
    }
    pos--;
  }
  return x;
}

/********************************************************************/
/* Operators that require a square root computation */

IVint32 iv_magnitude_v2i32(IVVec2D_i32 *a)
{
  return iv_sqrt_i64(iv_dot2_v2i32(a, a));
}

/* Approximate magnitude, faster but less accurate.  */
IVint32 iv_magn_v2i32(IVVec2D_i32 *a)
{
  return iv_aprx_sqrt_i64(iv_dot2_v2i32(a, a));
}

/* Vector normalization, convert to a Q16.16 fixed-point
   representation.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVVec2D_i32 *iv_normalize2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  IVint32 d = iv_magnitude_v2i32(a);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  return iv_shldiv4_v2i32_i32(a, a, 0x10, d);
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

/* Eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVVec2D_i32 *iv_elim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVint32 d;
  IVVec2D_i32 t;
  d = iv_magnitude_v2i32(c);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  iv_sub3_v2i32
    (a, b,
     iv_muldiv4_v2i32_i64_i32
       (&t, c, iv_dot2_v2i32(b, c), d)
    );
  return a;
}

/* Approximate eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.
*/
IVVec2D_i32 *iv_aelim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVint32 d;
  IVVec2D_i32 t;
  d = iv_magn_v2i32(c);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  iv_sub3_v2i32
    (a, b,
     iv_muldiv4_v2i32_i64_i32
       (&t, c, iv_dot2_v2i32(b, c), d)
    );
  return a;
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
IVint32 iv_dist2_v2i32_Eqs_v2i32(IVVec2D_i32 *a, IVEqs_v2i32 *b)
{
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
IVint32 iv_adist2_v2i32_Eqs_v2i32(IVVec2D_i32 *a, IVEqs_v2i32 *b)
{
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
IVint32 iv_dist2_v2i32_Eqs_v2q16i32(IVVec2D_i32 *a, IVEqs_v2i32 *b)
{
  return (IVint32)((iv_dot2_v2i32(a, &b->v) >> 0x10) - b->offset);
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
IVint32 iv_dist2_v2i32_NRay_v2i32(IVVec2D_i32 *a, IVNLine_v2i32 *b)
{
  IVint32 d;
  IVVec2D_i32 l_rel_p;
  d = iv_magnitude_v2i32(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  iv_sub3_v2i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v2i32(&l_rel_p, &b->v) / d);
}

/* Approximate variant of the previous subroutine.  */
IVint32 iv_adist2_v2i32_NRay_v2i32(IVVec2D_i32 *a, IVNLine_v2i32 *b)
{
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
  return iv_dot2_v2i32(&t, &t);
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

   L - A * (dot_product(L, A) - d) / dot_product(A, A)
*/
IVPoint2D_i32 *iv_proj3_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					IVEqs_v2i32 *c)
{
  IVVec2D_i32 t;
  /* TODO VERIFY PRECISION: offset is only 32 bits? */
  iv_muldiv4_v2i32_i64(&t, &c->v, iv_dot2_v2i32(b, &c->v) - c->offset,
		       iv_dot2_v2i32(&c->v, &c->v));
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
   L - A * dot_product(L_rel_P, A) / dot_product(A, A);
*/
IVPoint2D_i32 *iv_proj3_p2i32_NLine_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					  IVNLine_v2i32 *c)
{
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  iv_sub3_v2i32(&l_rel_p, b, &c->p0);
  iv_muldiv4_v2i32_i64(&t, &c->v, iv_dot2_v2i32(&l_rel_p, &c->v),
		       iv_dot2_v2i32(&c->v, &c->v));
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
*/
IVPoint2D_i32 *iv_isect3_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					  IVInLine_v2i32 *b,
					  IVEqs_v2i32 *c)
{
  IVVec2D_i32 t;
  IVint64 d;
  d = iv_dot2_v2i32(&b->v, &c->v) >> g_shr;
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  iv_muldiv4_v2i32_i64(&t, &b->v,
		       (iv_dot2_v2i32(&b->p0, &c->v) >> g_shr) - c->offset, d);
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
*/
IVPoint2D_i32 *iv_isect3_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVNLine_v2i32 *c)
{
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  IVint64 d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
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
  IVVec2D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  n = -iv_dot2_v2i32(&b->p0, &c->v) + c->offset;
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
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
  IVVec2D_i32 l_rel_p;
  IVVec2D_i32 t;
  IVint64 n, d;
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  iv_sub3_v2i32(&l_rel_p, &b->p0, &c->p0);
  n = -iv_dot2_v2i32(&l_rel_p, &c->v);
  /* Bit-wise equivalent of (n / d) < 0 */
  if (((n & 0x8000000000000000LL) ^ (d & 0x8000000000000000LL)) != 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
  iv_muldiv4_v2i32_i64(&t, &b->v, n, d);
  return iv_add3_v2i32(a, &b->p0, &t);
}

/* Convert (reformat) Eqs representational form to NLine.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVNLine_v2i32 *iv_rf_NLine_Eqs_v2i32(IVNLine_v2i32 *a, IVEqs_v2i32 *b)
{
  /* TODO VERIFY PRECISION: offset is only 32 bits?  */
  /* Convert the origin offset to a point.  */
  IVint64 d = iv_dot2_v2i32(&b->v, &b->v) >> g_shr;
  if (d == 0) {
    /* No solution.  */
    a->p0.d[IX] = IVINT32_MIN;
    a->p0.d[IY] = IVINT32_MIN;
    a->v.d[IX] = IVINT32_MIN;
    a->v.d[IY] = IVINT32_MIN;
    return a;
  }
  iv_muldiv4_v2i32_i64
    (&a->p0, &b->v, b->offset, d);
  /* Copy the perpendicular vector.  */
  a->v = b->v;
  return a;
}

/* Convert (reformat) NLine representational form to Eqs.  */
IVEqs_v2i32 *iv_rf_Eqs_NLine_v2i32(IVEqs_v2i32 *a, IVNLine_v2i32 *b)
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
  a->offset = (IVint32)iv_dot2_v2i32(&t, &b->v);
  /* TODO VERIFY PRECISION: offset is only 32 bits? */
  return a;
}

/* Convert (reformat) InLine representational form to NLine.  */
IVNLine_v2i32 *iv_rf_NLine_InLine_v2i32(IVNLine_v2i32 *a, IVInLine_v2i32 *b)
{
  /* Copy the location.  */
  a->p0 = b->p0;
  /* Compute the perpendicular vector to use for the NLine.  */
  a->v.d[IX] = -b->v.d[IY];
  a->v.d[IY] = b->v.d[IX];
  return a;
}

/* Convert (reformat) NLine representational form to InLine.  */
IVNLine_v2i32 *iv_rf_InLine_NLine_v2i32(IVInLine_v2i32 *a, IVNLine_v2i32 *b)
{
  /* Exactly the same code as the reverse conversion, just different
     types.  */
  return iv_rf_NLine_InLine_v2i32((IVNLine_v2i32*)a, (IVInLine_v2i32*)b);
}

/* Solve a system of two simple linear equations, i.e. Ax = b format.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_Eqs_v2i32(IVPoint2D_i32 *a, IVSys2_Eqs_v2i32 *b)
{
  IVNLine_v2i32 nline;
  IVInLine_v2i32 in_line;
  /* Reformat the first equation into a InLine equation.  */
  iv_rf_NLine_Eqs_v2i32(&nline, &b->d[0]);
  iv_rf_InLine_NLine_v2i32(&in_line,  &nline);
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
  iv_rf_InLine_NLine_v2i32(&in_line, &b->d[0]);
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
  iv_rf_InLine_NLine_v2i32(&nline, &b->d[1]);
  /* Now intersect the InLine with the plane (line in 2D).  */
  return iv_isect3_InLine_NLine_v2i32(a, &b->d[0], &nline);
}

/* Multiply, shift right two MxN integer matrices, with symmetric
   positive/negative shift behavior.  After each element
   multiplication, the result is shifted right by `n` bits before
   adding to the accumulator.  The output matrix must be
   pre-allocated.

   If there is no solution, the resulting matrix entries are all set
   to IVINT32_MIN.  */
IVMatNxM_i32 *iv_mulshr4_mnxm_i32(IVMatNxM_i32 *a,
				  IVMatNxM_i32 *b, IVMatNxM_i32 *c,
				  IVuint32 d)
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
	  if (d != 0 && intermed < 0) intermed++;
	  accum += intermed >> d;
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

/* Compute a linear regression of the "y = m*x + b" representational
   form.  Result vector format is `[b, m]`.  Input integers must be in
   Q16.16 fixed-point format, output is also in Q16.16 fixed-point
   format.

   If there is no solution, the resulting matrix entries are all set
   to IVINT32_MIN.  */
IVPoint2D_i32 *iv_linreg2_p2q16i32(IVPoint2D_i32 *result,
				   IVPoint2D_i32_array *data)
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
  IVSys2_Eqs_v2i32 sys;
  IVuint16 i;

  /* Pre-allocate all intermediate data structures.  */
  mat_a.width = 2;
  mat_a.height = data_len;
  mat_a.d = (IVint32*)malloc(sizeof(IVint32) * mat_a.width * mat_a.height);
  /* TODO: Signal memory error.  */
  if (mat_a.d == NULL)
    return result;

  mat_a_t.width = mat_a.height;
  mat_a_t.height = mat_a.width;
  mat_a_t.d =
    (IVint32*)malloc(sizeof(IVint32) * mat_a_t.width * mat_a_t.height);
  /* TODO: Signal memory error.  */
  if (mat_a_t.d == NULL) {
    free (mat_a.d);
    return result;
  }

  col_b.width = 1;
  col_b.height = data_len;
  col_b.d =
    (IVint32*)malloc(sizeof(IVint32) * col_b.width * col_b.height);
  /* TODO: Signal memory error.  */
  if (col_b.d == NULL) {
    free (mat_a.d);
    free (mat_a_t.d);
    return result;
  }

  mat_a_t_a.width = 2;
  mat_a_t_a.height = 2;
  mat_a_t_a.d = mat_a_t_a_stor.d;
  col_a_t_b.width = 1;
  col_a_t_b.height = 2;
  col_a_t_b.d = col_a_t_b_stor.d;

  /* Pack into the coefficient matrix `A`.  */
  for (i = 0; i < data_len; i++) {
    mat_a.d[mat_a.width*i+0] = 1 << 0x10 >> 0x10;
    /* N.B.: Trying to subtract 100 for better numerical stability.  */
    mat_a.d[mat_a.width*i+1] = (data_d[i].d[IX] >> 0x10) /* - 100 */;
  }
  /* Compute `A^T`.  */
  iv_xpose2_mnxm_i32(&mat_a_t, &mat_a);

  /* Pack into the coefficient matrix `b`.  */
  for (i = 0; i < data_len; i++) {
    col_b.d[i] = data_d[i].d[IY] >> 0x10;
  }

  /* Compute `A^T * A`.  */
  iv_mulshr4_mnxm_i32(&mat_a_t_a, &mat_a_t, &mat_a, 0 /* 0x10 */);

  /* Compute `A^T * b`.  */
  iv_mulshr4_mnxm_i32(&col_a_t_b, &mat_a_t, &col_b, 0 /* 0x10 */);

  /* Pack into IVSys2_Eqs_v2i32 representational form.  */
  sys.d[0].v.d[IX] = mat_a_t_a.d[0];
  sys.d[0].v.d[IY] = mat_a_t_a.d[1];
  sys.d[0].offset = col_a_t_b.d[0];
  sys.d[1].v.d[IX] = mat_a_t_a.d[2];
  sys.d[1].v.d[IY] = mat_a_t_a.d[3];
  sys.d[1].offset = col_a_t_b.d[1];

  printf("%d %d %d\n", sys.d[0].v.d[IX], sys.d[0].v.d[IY], sys.d[0].offset);
  printf("%d %d %d\n", sys.d[1].v.d[IX], sys.d[1].v.d[IY], sys.d[1].offset);
  printf("\n");
  /* Now, here's the trick.  We want to analyze the slopes of the line
     equations to determine how much we can divide the numbers, up to
     our shift limit.  You can divide all equations and get the same
     result, so long as the sub-computations do proper decimal place
     handling.

     For slopes less than one, double the slope is approximately
     double the angle.  But for slopes much greater than one, double
     the slope is pretty much the exact same angle, which is why it's
     important to divide when using a geometric equation solver.

     Absolutely ideal behavior requires us to do division computations
     rather than mere bit shifting.  */
  {
    IVuint8 num_bits = soft_bsr_i64(sys.d[0].v.d[IY]);
    IVuint8 shr_div = num_bits;
    printf("%d\n", num_bits);
    g_shr = 0x08;
    if (shr_div > g_shr)
      shr_div = g_shr;
    printf("%d\n", shr_div);
    sys.d[0].v.d[IX] <<= g_shr - shr_div;
    sys.d[0].v.d[IY] <<= g_shr - shr_div;
    sys.d[0].offset <<= g_shr - shr_div;
    sys.d[1].v.d[IX] <<= g_shr - shr_div;
    sys.d[1].v.d[IY] <<= g_shr - shr_div;
    sys.d[1].offset <<= g_shr - shr_div;
  printf("%d %d %d\n", sys.d[0].v.d[IX], sys.d[0].v.d[IY], sys.d[0].offset);
  printf("%d %d %d\n", sys.d[1].v.d[IX], sys.d[1].v.d[IY], sys.d[1].offset);
  printf("\n");
  }

  /* Solve the system!  */
  iv_solve2_s2_Eqs_v2i32(result, &sys);

  /* Cleanup.  */
  free (mat_a.d);
  free (mat_a_t.d);
  free (col_b.d);

  return result;
}
