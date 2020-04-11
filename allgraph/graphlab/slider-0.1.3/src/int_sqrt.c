/* Iterative integer square root computation.

(x + n)^2
= x^2 + 2*x*n + n^2

So how do you use this equation to compute a square root?  Basically,
you want to iteratively come closer to the actual solution.

So this is how it's done.

Starting from the most significant bit, you want to guess the value of
each bit progressively.  That is your `n`.  Your current running
approximation of the square root is your `x`.

Initialize `x` by using the bit-wise approximate square root
operation: bit-scan reverse to determine the most significant bit.
Count the number of places after that bit, and divide by two.  Then
shift right the number "1" by that number of places.  That is your
approximate square root.

Next, use the `x^2 + 2*x*n + n^2` equation to iteratively step closer
to your integer square root.  You will start out with an underestimate
and grow larger, until you've guessed all bits.

*/

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int IVuint32;
typedef int IVint32;
typedef long long IVint64;
#define IVINT32_MIN ((IVint32)0x80000000)

/* Fallback implementation of bit-scan reverse in software.  */
IVuint32 soft_bsr_i64(IVint64 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  /* TODO FIXME: That's what I did wrong!  I should do a sequential
     search when we get down to only 4 entries remaining!  */
  IVint64 mask = 0xffffffff00000000LL;
  IVuint32 shift = 0x10;
  IVuint32 pos = 0x20;
  IVuint32 dir = 0;
  while (shift > 0) {
    if ((a & mask)) {
      /* Look more carefully to the left.  */
      mask <<= shift;
      pos += shift;
      shift >>= 1;
      dir = 1;
    } else {
      /* Look more carefully to the right.  */
      mask >>= shift;
      pos -= shift;
      shift >>= 1;
      dir = (IVuint32)-1;
    }
  }
  /* Now we need to find out if we need to go to the right by one or
     just stay where we are.  */
  if (!(a & mask))
    pos--;
  return pos;
}

/* Fallback implementation of bit-scan reverse in software, but
   doesn't use any shifting instructions at all!  This is useful if
   your CPU does not have bit-shifting instructions (Gigatron), or it
   doesn't support multi-bit shifts (i.e. 6502).  */
IVuint32 soft_ns_bsr_i64(IVint64 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  /* TODO FIXME: That's what I did wrong!  I should do a sequential
     search when we get down to only 4 entries remaining!  */
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
  static const IVuint32 shifts[6] =
    { 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
  IVuint32 shift_idx = 0;
  IVuint32 pos = 0x20;
  IVuint32 dir = 0;
  while (shift_idx < 5) {
    if ((a & masks[pos])) {
      /* Look more carefully to the left.  */
      pos += shifts[shift_idx++];
      dir = 1;
    } else {
      /* Look more carefully to the right.  */
      pos -= shifts[shift_idx++];
      dir = (IVuint32)-1;
    }
  }
  /* Now we need to find out if we need to go to the right by one or
     just stay where we are.  */
  if (!(a & masks[pos]))
    pos--;
  return pos;
}

/* Compute an approximate square root by using bit-scan reverse and
   dividing the number of significant bits by two.  Yes, we shift
   right to divide by two.  If the operand is negative, IVINT32_MIN is
   returned since there is no solution.

   Please note that when you square the result of the approximate
   square root, it can fall short of the actual number squared by up
   to a factor of two.  So that means the result may be multiplied by
   a factor of sqrt(0.5) ~= 0.707 compared to the actual value.

   But, by all means, if you only need an order of magnitude estimate,
   this method is plenty fine.  */
IVint32 iv_aprx_sqrt_i64(IVint64 a)
{
  IVuint32 num_sig_bits;
  if (a == 0)
    return 0;
  if (a < 0)
    return IVINT32_MIN;
  num_sig_bits = soft_bsr_i64(a);
  return 1 << (num_sig_bits >> 1);
}

/* (x + n)^2 = x^2 + 2*x*n + n^2 */
/* This method is designed to guarantee an underestimate of the square
   root, i.e. the fractional part is truncated as is the case with
   standard integer arithmetic.  */
IVint32 iv_sqrt_i64(IVint64 a)
{
  IVuint32 pos;
  IVint32 x;
  IVint64 x2;
  if (a == 0)
    return 0;
  if (a < 0)
    return IVINT32_MIN;
  pos = soft_bsr_i64(a) >> 1;
  x = 1 << pos;
  x2 = 1 << (pos << 1);
  pos--;
  while (pos != (IVuint32)-1) {
    IVint32 n = 1 << pos; /* n == 2^pos */
    IVint64 n2 = 1 << (pos << 1); /* n^2 */
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

int main(int argc, char *argv[])
{
  IVint32 input = 0, out_approx_sqrt, out_sqrt;
  if (argc >= 2)
    input = atoi(argv[1]);
  out_approx_sqrt = iv_aprx_sqrt_i64(input);
  out_sqrt = iv_sqrt_i64(input);
  printf("in == %d, aprx == %d, sqrt == %d\n",
	 input, out_approx_sqrt, out_sqrt);
  printf("diff aprx == %d, sqrt == %d\n",
	 input - out_approx_sqrt * out_approx_sqrt,
	 input - out_sqrt * out_sqrt);
  return 0;
}
