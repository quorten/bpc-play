/* Simple integer vector math subroutines.  A second iteration on my
   previous complicated generics design.  We only support 32-bit
   integers with 64-bit intermediates, two dimensions, and three
   dimensions.

   Fixed-point arithmetic is only used as an intermediate form,
   Q32.32.  */

#include "ivecmath.h"

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

/* shift right, with symmetric positive/negative shift behavior */
IVVec2D_i32 *iv_shr3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint32 c)
{
  IVint32 tx = b->d[IX], ty = b->d[IY];
  if (c == 0)
    return a;
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
/* Operators that require a square root computation */

#if 0

/* N.B.: Approximate square root counts leading bits using
   bit-scanning CPU instructions (when present) and then figures out
   how many bits to shift that will result in half as many significant
   figures.  */

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
   representation.  */
IVVec2D_i32 *iv_normalize2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b)
{
  return iv_shldiv4_v2i32_i32(a, a, 0x10, iv_magnitude_v2i32(a));
}

/* Eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)
*/
IVVec2D_i32 *iv_elim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c)
{
  IVVec2D_i32 t;
  iv_sub3_v2i32
    (a, b,
     iv_muldiv4_v2i32_i64_i32
       (&t, c, iv_dot2_v2i32(b, c), iv_magnitude_v2i32(c))
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
*/
IVint32 iv_dist2_v2i32_Eqs_v2i32(IVVec2D_i32 *a, IVEqs_v2i32 *b)
{
  return (IVint32)((iv_dot2_v2i32(a, &b->v) - b->offset) /
		   iv_magnitude_v2i32(&b->v));
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
*/
IVint32 iv_dist2_v2i32_NRay_v2i32(IVVec2D_i32 *a, IVNRay_v2i32 *b)
{
  IVVec2D_i32 l_rel_p;
  iv_sub3_v2i32(&l_rel_p, a, &b->p0);
  return (IVint32)(iv_dot2_v2i32(&l_rel_p, &b->v) /
		   iv_magnitude_v2i32(&b->v));
}

#endif

/********************************************************************/

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
  d = iv_dot2_v2i32(&b->v, &c->v);
  if (d == 0) {
    /* No solution.  */
    a->d[IX] = IVINT32_MIN;
    a->d[IY] = IVINT32_MIN;
    return a;
  }
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

/* Solve a system of two simple linear equations, i.e. Ax = b format.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_Eqs_v2i32(IVPoint2D_i32 *a, IVSys2_Eqs_v2i32 *b)
{
  IVVec2D_i32 *v0 = &b->d[0].v;
  IVRay_v2i32 ray;
  /* TODO VERIFY PRECISION: offset is only 32 bits? */
  /* Reformat the first equation into a ray equation.  */
  iv_muldiv4_v2i32_i64
    (&ray.p0, &b->d[0].v, b->d[0].offset, iv_dot2_v2i32(v0, v0));
  /* Compute a perpendicular vector to use for the ray.  */
  ray.v.d[IX] = -v0->d[IY];
  ray.v.d[IY] = v0->d[IX];
  /* Now intersect the ray with the plane (line in 2D).  */
  return iv_isect3_InLine_Eqs_v2i32(a, &ray, &b->d[0]);
}

/* Solve a system of two "surface-normal" perpendicular vector linear
   equations.

   If there is no solution, the resulting point's coordinates are all
   set to IVINT32_MIN.  */
IVPoint2D_i32 *iv_solve2_s2_NLine_v2i32(IVPoint2D_i32 *a,
					IVSys2_NLine_v2i32 *b)
{
  IVVec2D_i32 *v0 = &b->d[0].v;
  IVRay_v2i32 ray;
  ray.p0 = b->d[0].p0;
  /* Compute a perpendicular vector to use for the ray.  */
  ray.v.d[IX] = -v0->d[IY];
  ray.v.d[IY] = v0->d[IX];
  /* Now intersect the ray with the plane (line in 2D).  */
  return iv_isect3_InLine_NLine_v2i32(a, &ray, &b->d[1]);
}
