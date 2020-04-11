/* Simple integer vector math subroutines.  A second iteration on my
   previous complicated generics design.  We only support 32-bit
   integers with 64-bit intermediates, two dimensions, and three
   dimensions.

   Fixed-point arithmetic is only used as an intermediate form,
   Q32.32.  */

#ifndef IVECMATH_H
#define IVECMATH_H

/* Coordinate indices */
typedef enum IVPtIdx_tag IVPtIdx;
enum IVPtIdx_tag { IX, IY, IZ };

/* Fully fixed array.  Both the allocation and the length of the array
   are fixed and identical.  This is mainly some helper macros to make
   generics programming easier.  */
#define FFA_TYPE(typename, length) \
  struct typename##_ar##length##_tag \
  { \
    typename d[length]; \
  }; \
  typedef struct typename##_ar##length##_tag typename##_ar##length

#define EA_DEF_TYSIZE
#define EA_TYPE(typename)						\
  struct typename##_array_tag						\
  {									\
    typename *d;							\
    unsigned len;							\
    EA_DEF_TYSIZE							\
    /* User-defined fields.  */						\
    unsigned user1;							\
  };									\
  typedef struct typename##_array_tag typename##_array

typedef unsigned int IVuint32;
typedef int IVint32;
typedef long long IVint64;

#define IVINT32_MIN ((IVint32)0x80000000)

FFA_TYPE(IVint32, 2);
FFA_TYPE(IVint32, 3);
FFA_TYPE(IVint32, 4);
FFA_TYPE(IVint32, 9);
/* {2,3}-dimensional vectors and points */
typedef IVint32_ar2 IVVec2D_i32;
typedef IVint32_ar2 IVPoint2D_i32;
typedef IVint32_ar3 IVVec3D_i32;
typedef IVint32_ar3 IVPoint3D_i32;
/* {2,3}x{2,3} matrices */
typedef IVint32_ar4 IVMat2x2_i32;
typedef IVint32_ar9 IVMat3x3_i32;
/* {2,3}x{2,3} transposed matrices */
typedef IVint32_ar4 IVTMat2x2_i32;
typedef IVint32_ar9 IVTMat3x3_i32;

FFA_TYPE(IVPoint2D_i32, 2);
FFA_TYPE(IVPoint3D_i32, 2);
FFA_TYPE(IVPoint3D_i32, 3);
/* Line segment */
typedef IVPoint2D_i32_ar2 IVLineSeg_p2i32;
typedef IVPoint3D_i32_ar2 IVLineSeg_p3i32;
/* Triangle */
typedef IVPoint3D_i32_ar3 IVTri_p3i32;

FFA_TYPE(IVuint32, 2);
FFA_TYPE(IVuint32, 3);
/* Line segment from indices */
typedef IVuint32_ar2 IVLineSeg_u32;
/* Triangle from indices */
typedef IVuint32_ar3 IVTri_u32;

/* Simple linear equation */
struct IVEqs_v2i32_tag
{
  IVVec2D_i32 v; /* "left" side of vector equation */
  IVint32 offset; /* "right" side of vector equation */
};
typedef struct IVEqs_v2i32_tag IVEqs_v2i32;
struct IVEqs_v3i32_tag
{
  IVVec3D_i32 v; /* "left" side of vector equation */
  IVint32 offset; /* "right" side of vector equation */
};
typedef struct IVEqs_v3i32_tag IVEqs_v3i32;

/* N.B. A simple linear equation can also function as the equation of
   a line, the equation of a plane, etc.  */

/* Abstract ray: [0] direction vector, [1] ray origin ("point zero") */
struct IVRay_v2i32_tag
{
  IVVec2D_i32 v; /* "left" side of vector equation */
  IVPoint2D_i32 p0; /* "right" side of vector equation */
};
typedef struct IVRay_v2i32_tag IVRay_v2i32;
struct IVRay_v3i32_tag
{
  IVVec3D_i32 v; /* "left" side of vector equation */
  IVPoint3D_i32 p0; /* "right" side of vector equation */
};
typedef struct IVRay_v3i32_tag IVRay_v3i32;

/* N.B. Note that the ray data structure can also function as a line
   equation, a plane equation, and therefore even an equation in a
   system of linear equations.  */

/* Equation of a line defined by a perpendicular vector, i.e. "surface
   normal."  */
typedef IVRay_v2i32 IVNLine_v2i32;
/* Equation of a line defined by an in-line vector.  */
typedef IVRay_v2i32 IVInLine_v2i32;
/* Equation of a plane defined by a surface normal vector and an
   in-plane point.  */
typedef IVRay_v3i32 IVNPlane_v3i32;
/* Equation of a plane defined by two in-plane vectors and an in-plane
   point.  */
struct IVInPlane_v3i32_tag
{
  IVVec3D_i32 v0;
  IVVec3D_i32 v1;
  IVPoint3D_i32 p0;
};
typedef struct IVInPlane_v3i32_tag IVInPlane_v3i32;

/* Dimensional systems of {2,3} equations */
FFA_TYPE(IVEqs_v2i32, 2);
FFA_TYPE(IVEqs_v3i32, 3);
FFA_TYPE(IVNLine_v2i32, 2);
FFA_TYPE(IVInLine_v2i32, 2);
FFA_TYPE(IVNPlane_v3i32, 3);
FFA_TYPE(IVInPlane_v3i32, 3);
typedef IVEqs_v2i32_ar2 IVSys2_Eqs_v2i32;
typedef IVEqs_v3i32_ar3 IVSys3_Eqs_v3i32;
typedef IVNLine_v2i32_ar2 IVSys2_NLine_v2i32;
typedef IVInLine_v2i32_ar2 IVSys2_InLine_v2i32;
typedef IVNPlane_v3i32_ar3 IVSys3_NPlane_v3i32;
typedef IVInPlane_v3i32_ar3 IVSys3_InPlane_v3i32;

/********************************************************************/

/* Common arbitrary length array types */
/* Point cloud */
EA_TYPE(IVPoint2D_i32);
EA_TYPE(IVLineSeg_u32);
EA_TYPE(IVTri_u32);
/* Line segments from indices */
struct IVLineSegs_p2u32_tag
{
  IVPoint2D_i32_array points;
  IVLineSeg_u32_array lines;
};
typedef struct IVLineSegs_p2u32_tag IVLineSegs_p2u32;
/* Triangles (mesh) from indices */
struct IVTris_p2u32_tag
{
  IVPoint2D_i32_array points;
  IVTri_u32_array tris;
};
typedef struct IVTris_p2u32_tag IVTris_p2u32;

/********************************************************************/

IVVec2D_i32 *iv_neg2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b);
IVVec2D_i32 *iv_add3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c);
IVVec2D_i32 *iv_sub3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c);
IVVec2D_i32 *iv_muldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVint32 d);
IVVec2D_i32 *iv_muldiv4_v2i32_i64(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint64 c, IVint64 d);
IVVec2D_i32 *iv_muldiv4_v2i32_i64_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				      IVint64 c, IVint32 d);
IVVec2D_i32 *iv_shldiv4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVuint32 c, IVint32 d);
IVVec2D_i32 *iv_mulshr4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVuint32 d);
IVVec2D_i32 *iv_shr3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint32 c);
IVint64 iv_dot2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b);

IVPoint2D_i32 *iv_proj3_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_proj3_p2i32_NLine_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					  IVNLine_v2i32 *c);
IVPoint2D_i32 *iv_isect3_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					  IVInLine_v2i32 *b,
					  IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_isect3_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVNLine_v2i32 *c);
IVPoint2D_i32 *iv_isect3_Ray_Eqs_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
				       IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_isect3_Ray_NLine_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
					 IVNLine_v2i32 *c);
IVNLine_v2i32 *iv_rf_NLine_Eqs_v2i32(IVNLine_v2i32 *a, IVEqs_v2i32 *b);
IVEqs_v2i32 *iv_rf_Eqs_NLine_v2i32(IVEqs_v2i32 *a, IVNLine_v2i32 *b);
IVNLine_v2i32 *iv_rf_NLine_InLine_v2i32(IVNLine_v2i32 *a, IVInLine_v2i32 *b);
IVNLine_v2i32 *iv_rf_InLine_NLine_v2i32(IVInLine_v2i32 *a, IVNLine_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_Eqs_v2i32(IVPoint2D_i32 *a, IVSys2_Eqs_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_NLine_v2i32(IVPoint2D_i32 *a,
					IVSys2_NLine_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_InLine_v2i32(IVPoint2D_i32 *a,
					 IVSys2_InLine_v2i32 *b);

#endif /* not IVECMATH_H */