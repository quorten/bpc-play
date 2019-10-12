/* Simple library for manipulating "clamped rational numbers."

   That is, in particular, the denominator is clamped to a maximum
   value so as to affect a certain type of "fixed point" number where
   only a maximum number of bits after the decimal point are
   allowed.  */

struct ClampRat16_tag
{
  short num; /* numerator */
  short dem; /* denominator */
};
typedef struct ClampRat16 ClampRat16;

struct ClampRat32_tag
{
  int num; /* numerator */
  int dem; /* denominator */
};
typedef struct ClampRat16 ClampRat32;

/* TODO FIXME: Create  a faster method for determining  the bit length
   of a number.  */
unsigned
bitlen (unsigned num)
{
  unsigned i = 0;
  while (num > 0) {
    num >>= 1;
    i++;
  }
  return i;
}

/* TODO: Approximate square root by dividing bit length by two.  */

void
clamprat16_mul (ClampRat16 accum, ClampRat16 b)
{
  unsigned shval = 0;
  accum.dem *= b.dem;
  if (accum.dem > 0xff) {
    /* Compute the shift value required to clamp the denominator.  */
    shval = bitlen (accum.dem) - 8;
    accum.dem >>= shval;
  }
  accum.num = (short) (((int) accum * b) >> shval);
}

void
clamprat16_recip (ClampRat16 accum)
{
  short temp = accum.dem;
  accum.dem = accum.num;
  accum.num = temp;
  /* TODO: Check if we are still clamped.  */
}

void
clamprat16_normalize (ClampRat16 accum)
{
  if (accum.dem < 0) {
    accum.num = -accum.num;
    accum.dem = -accum.dem;
  }
}

void
clamprat16_set (ClampRat16 accum, short num, short dem)
{
  accum.num = num;
  accum.dem = dem;
}

void
clamprat16_add (ClampRat16 accum, ClampRat16 b)
{
}

/* N.B. Where there is not idiv, rational arithmetic is faster for
   small matrices.  For large matrices, and definitely where there is
   hardware idiv, fixed point is faster.  */
