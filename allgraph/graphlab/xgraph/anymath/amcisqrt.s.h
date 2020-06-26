/* Math subroutines for square roots on built-in C integer data types.
   If this is a ".s.h" file, then it is generalized to all C integer
   data types.  */

#ifndef AM_CSQRT_ABNumx1
#define AM_CSQRT_ABNumx1

ABUNumx1 ab_aprx_sqrt1_brunumx2(ABUNumx2 a);
#define ab_aprx_sqrt2_brunumx2(a, b) ((a) = ab_aprx_sqrt1_brunumx2(b))
ABNumx1 ab_aprx_sqrt1_brnumx2(ABNumx2 a);
#define ab_aprx_sqrt2_brnumx2(a, b) ((a) = ab_aprx_sqrt1_brnumx2(b))
ABBitIdx ab_aprx_sqrt1_log2_brunumx2(ABUNumx2 a);
#define ab_aprx_sqrt2_log2_brunumx2(a, b) \
  ((a) = ab_aprx_sqrt1_log2_brunumx2(b))

int ab_init_sqrt_lut0_brunumx1(void);
void ab_destroy_sqrt_lut0_brunumx1(void);
ABNumx1 ab_sqrt_lut1_brunumx1(ABUNumx1 a);
ABNumx1 ab_sqrt_lut1_brnumx1(ABNumx1 a);
ABUNumx1 ab_sqrt1_brunumx2(ABUNumx2 a);
ABNumx1 ab_sqrt1_brnumx2(ABNumx2 a);

#endif /* not AM_CSQRT_ABNumx1 */
