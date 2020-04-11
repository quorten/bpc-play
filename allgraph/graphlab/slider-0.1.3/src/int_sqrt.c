/*

Integer square root computation routines.  There are plenty of tricks
that are used to optimize performance.

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

#include <stdio.h>
#include <stdlib.h>

typedef unsigned int IVuint32;
typedef int IVint32;
typedef long long IVint64;
#define IVINT32_MIN ((IVint32)0x80000000)

static IVuint32 *sqrt_lut = NULL;

/* Fallback implementation of bit-scan reverse in software.  */
IVuint32 soft_bsr_i64(IVint64 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  IVint64 mask = 0xffffffff00000000LL;
  IVuint32 shift = 0x10;
  IVuint32 pos = 0x20;
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
IVuint32 soft_ns_bsr_i64(IVint64 a)
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
  static const IVuint32 shifts[6] =
    { 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
  IVuint32 shift_idx = 0;
  IVuint32 pos = 0x20;
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
   square root computations.  */
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
  IVuint32 pos = 0x8000;
  IVuint32 shift = 0x4000;
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
  IVuint32 num_sig_bits;
  if (a == 0)
    return 0;
  if (a < 0)
    return IVINT32_MIN;
  num_sig_bits = soft_bsr_i64(a);
  return 1 << (num_sig_bits >> 1);
}

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
  if (a <= 0xffffffff && sqrt_lut != NULL) {
    return iv_sqrt_u32((IVuint32)a);
  }
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

  if (!init_sqrt_lut())
    puts("Error: Could not initialize square root lookup table.");
  out_approx_sqrt = iv_aprx_sqrt_i64(input);
  out_sqrt = iv_sqrt_i64(input);
  destroy_sqrt_lut();

  printf("in == %d, aprx == %d, sqrt == %d\n",
	 input, out_approx_sqrt, out_sqrt);
  printf("diff aprx == %d, sqrt == %d\n",
	 input - out_approx_sqrt * out_approx_sqrt,
	 input - out_sqrt * out_sqrt);
  return 0;
}
