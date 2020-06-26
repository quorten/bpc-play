/* Math subroutines for ABNumx1 built-in C integer data types,
   designed to be similar to GMP (GNU Multiple Precision) subroutine
   calling convention.  If this is a ".s.c" file, then it is
   generalized to all C integer data types.  */

#include "amcint.s.h"

/*

Template parameters:

* ABNumx1: Scalar number type, base width.

* ABBitIdx: Integer data type for referencing bit indexes up to the
  width of ABNumx2, i.e. unsigned char.  Must be a built-in C integer
  data type.  Used in base 2 logarithm (shifting) calculations.

* brnumx1: Abbreviated symbol for abstract number, base width, for use
  in higher level type names, functions, etc.

* brbitidx: Abbreviated symbol for bit index integer data type, for
  use in higher level type names, functions, etc.

* AB_NO_CPU_SHIFT: Define this if your CPU does not have multi-bit
  shift instructions.  Otherwise, leave undefined.

* LOG2_SZ_ABNumx1: Base 2 logarithm of ABNumx1 size in bits.

*/

static ABNumx1 fls_masks_brnumx1[8*sizeof(ABNumx1)];
/* TODO FIXME sizing, we have to be told in advance what
   log2(BIT_WIDTH) is to size this.  */
static ABBitIdx fls_shifts_brnumx1[LOG2_SZ_ABNumx1];

/* Fallback implementation of find last bit set in software.  */
ABBitIdx ab_sf_fls1_brnumx1(ABNumx1 a)
{
  /* Use a binary search tree of AND masks to determine where the most
     significant bit is located.  Note that with 16 bits or less, a
     sequential search is probably faster due to CPU branching
     penalties.  */
  ABNumx1 mask = ~ab_pow2mask1_brnumx1(8 * sizeof(ABNumx1) / 2);
  ABBitIdx shift = 8 * sizeof(ABNumx1) / 4;
  ABBitIdx pos = 8 * sizeof(ABNumx1) / 2;
  /* Check for special case if no bits are set.  */
  if (a == 0)
    return (ABBitIdx)-1;
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

/* Initialize the `fls` masks and shifts tables.  See
   `ab_sfns_fls1_brnumx1()` for more details of its use.  This is tough
   with generics programming, normally this would be generated before
   compile-time and linked in.  Nevertheless, it may be useful to
   generate at run-time simply because that means our compiled image
   is slightly smaller.

   Since we only shift left to initialize, this can be done
   efficiently even without shift instructions.  */
void ab_init_fls1_masks_tbl(void)
{
  ABBitIdx i;
  ABNumx1 cur_mask = ~(ABNumx1)0;
  for (i = 0; i < 8 * sizeof(ABNumx1); i++) {
    fls_masks_brnumx1[i] = cur_mask;
    cur_mask <<= 1;
  }
  i = LOG2_SZ_ABNumx1;
  i--;
  fls_shifts_brnumx1[i] = 0;
  cur_mask = 1;
  while (i > 0) {
    i--;
    fls_shifts_brnumx1[i] = cur_mask;
    cur_mask <<= 1;
  }
}

/* Fallback implementation of find last bit set in software, but
   doesn't use any shifting instructions at all!  This is useful if
   your CPU does not have bit-shifting instructions (Gigatron), or it
   doesn't support multi-bit shifts (i.e. 6502).  */
ABBitIdx ab_sfns_fls1_brnumx1(ABNumx1 a)
{
  /* Use a binary search tree of AND masks (`fls_masks_brnumx1`) to
     determine where the most significant bit is located.  Note that
     with 16 bits or less, a sequential search is probably faster due
     to CPU branching penalties.  */
  ABBitIdx shift_idx = 0;
  ABBitIdx pos = 8 * sizeof(ABNumx1) / 2;
  /* Check for special case if no bits are set.  */
  if (a == 0)
    return (ABBitIdx)-1;
  /* while (shift > 1) */
  while (shift_idx < LOG2_SZ_ABNumx1 - 2) {
    if ((a & fls_masks_brnumx1[pos])) {
      /* Look more carefully to the left.  */
      pos += fls_shifts_brnumx1[shift_idx++];
    } else {
      /* Look more carefully to the right.  */
      pos -= fls_shifts_brnumx1[shift_idx++];
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
    if ((a & fls_masks_brnumx1[pos]))
      return pos;
    pos--;
  }
  return pos;
}

/* Platform independent stub implementation for `fls' find last bit
   set.  If there are zero significant bits, the result is
   (ABBitIdx)-1.  */
ABBitIdx ab_fls1_brnumx1(ABNumx1 a)
{
#ifdef __GCC__
#if LOG2_SZ_ABNumx1 == 6
  return 64 - 1 - __builtin_clzll(a);
#elif LOG2_SZ_ABNumx1 == 5 || LOG2_SZ_ABNumx1 == 4 || LOG2_SZ_ABNumx1 == 3
  return 32 - 1 - __builtin_clz(a);
#else
  /* For now, we just error, but we could alternatively implement a
     software subroutine that uses `clzll` in repeated succession.  */
#error "No clz primitive matches integer width."
#endif
#elif defined(AB_NO_CPU_SHIFT)
  return ab_sfns_fls1_brnumx1(a);
#else
  return ab_sf_fls1_brnumx1(a);
#endif
}

/* Get the index of the most significant bit.  To process negative
   numbers, they are inverted and then leading zeros are skipped.  */
ABBitIdx ab_msbidx1_brnumx1(ABNumx1 a)
{
  return ab_fls1_brnumx1(ab_abs1_brnumx1(a));
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
ABNumx1 ab_fsyshr2_brnumx1(ABNumx1 a, ABBitIdx q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (a < 0) a += (1 << q) - 1;
  return a >> q;
}

/* Shift right with symmetric positive/negative shift behavior.  `q`
   must not be equal to zero, or else the results will be
   inaccurate.  "f' is for "faster."  */
ABNumx1 ab_symshr2_brnumx1(ABNumx1 a, ABBitIdx q)
{
  /* Avoid asymmetric two's complement behavior.  */
  if (q != 0 && a < 0) a += (1 << q) - 1;
  return a >> q;
}
