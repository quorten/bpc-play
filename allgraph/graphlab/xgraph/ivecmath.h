/* Simple integer vector math subroutines.  A second iteration on my
   previous complicated generics design.  We only support 32-bit
   integers with 64-bit intermediates, two dimensions, and three
   dimensions.

   Fixed-point arithmetic is transparently supported by most
   higher-level vector math operators.  */

#ifndef IVECMATH_H
#define IVECMATH_H

/* Coordinate indices */
enum IVPtIdx_tag { IX, IY, IZ };
typedef enum IVPtIdx_tag IVPtIdx;

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

typedef unsigned char IVuint8;
typedef unsigned short IVuint16;
typedef unsigned int IVuint32;
typedef unsigned long long IVuint64;
typedef signed char IVint8;
typedef int IVint32;
typedef long long IVint64;

typedef IVint32 IVint32q16; /* Q16.16 fixed-point */
typedef IVint64 IVint64q32; /* Q32.32 fixed-point */

#define IVINT32_MIN ((IVint32)0x80000000)
#define IVINT64_MIN ((IVint64)0x8000000000000000LL)

/* "Named array" (nar) of two IVint32 items */
struct IVint32_nar2_tag
{
  IVint32 x;
  IVint32 y;
};
typedef struct IVint32_nar2_tag IVint32_nar2;

/* "Named array" (nar) of three IVint32 items */
struct IVint32_nar3_tag
{
  IVint32 x;
  IVint32 y;
  IVint32 z;
};
typedef struct IVint32_nar3_tag IVint32_nar3;

FFA_TYPE(IVint32, 2);
FFA_TYPE(IVint32, 3);
FFA_TYPE(IVint32, 4);
FFA_TYPE(IVint32, 9);
/* {2,3}-dimensional vectors and points */
typedef IVint32_nar2 IVVec2D_i32;
typedef IVint32_nar2 IVVec2D_i32q16;
typedef IVint32_nar2 IVNVec2D_i32q16; /* normalized vector */
typedef IVint32_nar2 IVPoint2D_i32;
typedef IVint32_nar3 IVVec3D_i32;
typedef IVint32_nar3 IVVec3D_i32q16;
typedef IVint32_nar3 IVNVec3D_i32q16; /* normalized vector*/
typedef IVint32_nar3 IVPoint3D_i32;
/* {2,3}x{2,3} matrices */
typedef IVint32_ar4 IVMat2x2_i32;
typedef IVint32_ar9 IVMat3x3_i32;
/* {2,3}x{2,3} transposed matrices */
typedef IVint32_ar4 IVTMat2x2_i32;
typedef IVint32_ar9 IVTMat3x3_i32;

/* Simple linear equation */
struct IVEqs_v2i32_tag
{
  IVVec2D_i32 v; /* "left" side of vector equation */
  IVint64 offset; /* "right" side of vector equation */
  /* N.B.: If using fixed-point, make sure offset has double the
     number of bits after the decimal.  */
};
typedef struct IVEqs_v2i32_tag IVEqs_v2i32;
typedef IVEqs_v2i32 IVEqs_v2i32q16;
typedef IVEqs_v2i32 IVEqs_nv2i32q16; /* normalized vectors */
struct IVEqs_v3i32_tag
{
  IVVec3D_i32 v; /* "left" side of vector equation */
  IVint64 offset; /* "right" side of vector equation */
  /* N.B.: If using fixed-point, make sure offset has double the
     number of bits after the decimal.  */
};
typedef struct IVEqs_v3i32_tag IVEqs_v3i32;
typedef IVEqs_v3i32 IVEqs_v3i32q16;
typedef IVEqs_v3i32 IVEqs_nv3i32q16; /* normalized vectors */

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
typedef IVRay_v3i32 IVInLine_v3i32;
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
typedef IVEqs_v2i32_ar2 IVSys2_Eqs_v2i32q16;
typedef IVEqs_v3i32_ar3 IVSys3_Eqs_v3i32;
typedef IVEqs_v3i32_ar3 IVSys3_Eqs_v3i32q16;
typedef IVNLine_v2i32_ar2 IVSys2_NLine_v2i32;
typedef IVInLine_v2i32_ar2 IVSys2_InLine_v2i32;
typedef IVNPlane_v3i32_ar3 IVSys3_NPlane_v3i32;
typedef IVInPlane_v3i32_ar3 IVSys3_InPlane_v3i32;

/********************************************************************/

/* Common fixed-length array types */

FFA_TYPE(IVPoint2D_i32, 2);
FFA_TYPE(IVPoint3D_i32, 2);
FFA_TYPE(IVPoint2D_i32, 3);
FFA_TYPE(IVPoint3D_i32, 3);
/* Line segment */
typedef IVPoint2D_i32_ar2 IVLineSeg_p2i32;
typedef IVPoint3D_i32_ar2 IVLineSeg_p3i32;
/* 2D rectangle region */
typedef IVPoint2D_i32_ar2 IVRect_p2i32;
/* 3D rectangular prism region */
typedef IVPoint3D_i32_ar2 IVRect_p3i32;
/* Triangle */
typedef IVPoint2D_i32_ar3 IVTri_p2i32;
typedef IVPoint3D_i32_ar3 IVTri_p3i32;

FFA_TYPE(IVuint32, 2);
FFA_TYPE(IVuint32, 3);
/* Line segment from indices */
typedef IVuint32_ar2 IVLineSeg_u32;
/* Triangle from indices */
typedef IVuint32_ar3 IVTri_u32;

/********************************************************************/

/* Common arbitrary length array types */

/* nxm matrices and transposed matrices.  */
struct IVMatNxM_i32_tag
{
  IVint32 *d;
  IVuint16 width;
  IVuint16 height;
};
typedef struct IVMatNxM_i32_tag IVMatNxM_i32;

/* Point cloud */
EA_TYPE(IVPoint2D_i32);
EA_TYPE(IVPoint3D_i32);
EA_TYPE(IVLineSeg_u32);
EA_TYPE(IVTri_u32);
/* Line segments from indices */
struct IVLineSegs_p2u32_tag
{
  IVPoint2D_i32_array points;
  IVLineSeg_u32_array lines;
};
typedef struct IVLineSegs_p2u32_tag IVLineSegs_p2u32;
struct IVLineSegs_p3u32_tag
{
  IVPoint3D_i32_array points;
  IVLineSeg_u32_array lines;
};
typedef struct IVLineSegs_p3u32_tag IVLineSegs_p3u32;
/* Triangles (mesh) from indices */
struct IVTris_p2u32_tag
{
  IVPoint2D_i32_array points;
  IVTri_u32_array tris;
};
typedef struct IVTris_p2u32_tag IVTris_p2u32;
struct IVTris_p3u32_tag
{
  IVPoint3D_i32_array points;
  IVTri_u32_array tris;
};
typedef struct IVTris_p3u32_tag IVTris_p3u32;

/********************************************************************/

IVint32 iv_abs_i32(IVint32 a);
IVint64 iv_abs_i64(IVint64 a);
IVuint8 iv_max2_u8(IVuint8 a, IVuint8 b);
IVuint8 iv_max3_u8(IVuint8 a, IVuint8 b, IVuint8 c);
IVuint8 iv_msbidx_i32(IVint32 a);
IVuint8 iv_msbidx_i64(IVint64 a);
IVint32 iv_shl_i32(IVint32 a, IVuint8 q);
IVint64 iv_shl_i64(IVint64 a, IVuint8 q);
IVint32 iv_shr_i32(IVint32 a, IVuint8 q);
IVint64 iv_shr_i64(IVint64 a, IVuint8 q);
IVint32 iv_fsyshr2_i32(IVint32 a, IVuint8 q);
IVint64 iv_fsyshr2_i64(IVint64 a, IVuint8 q);
IVint32 iv_symshr2_i32(IVint32 a, IVuint8 q);
IVint64 iv_symshr2_i64(IVint64 a, IVuint8 q);

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
				  IVuint8 c, IVint32 d);
IVVec2D_i32 *iv_mulshr4_v2i32_i32(IVVec2D_i32 *a, IVVec2D_i32 *b,
				  IVint32 c, IVuint8 q);
IVVec2D_i32 *iv_shl3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 c);
IVVec2D_i32 *iv_shr3_v2i32_u32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 c);
IVint64 iv_dot2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b);
IVint64 iv_magn2q_v2i32(IVVec2D_i32 *a);
IVVec2D_i32 *iv_perpen2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b);
IVVec2D_i32 *iv_nosol_v2i32(IVVec2D_i32 *a);
IVuint8 iv_is_nosol_v2i32(IVVec2D_i32 *a);
IVuint8 iv_is_zero_v2i32(IVVec2D_i32 *a);

IVuint8 soft_fls_i64(IVint64 a);
IVuint8 soft_ns_fls_i64(IVint64 a);
int init_sqrt_lut(void);
void destroy_sqrt_lut(void);
IVint32 iv_sqrt_u32(IVuint32 a);
IVint32 iv_sqrt_i32(IVint32 a);
IVuint32 iv_aprx_sqrt_u64(IVuint64 a);
IVint32 iv_aprx_sqrt_i64(IVint64 a);
IVuint8 iv_aprx_sqrt_log2_u64(IVuint64 a);
IVuint32 iv_aprx_msqrt_u64(IVuint64 a);
IVint32 iv_aprx_msqrt_i64(IVint64 a);
IVuint32 iv_sqrt_u64(IVuint64 a);
IVint32 iv_sqrt_i64(IVint64 a);

IVint32 iv_magnitude_v2i32(IVVec2D_i32 *a);
IVint32 iv_magn_v2i32(IVVec2D_i32 *a);
IVint32 iv_magn_log2_v2i32(IVVec2D_i32 *a);
IVuint8 iv_amagn_log2_v2i32(IVVec2D_i32 *a);
IVNVec2D_i32q16 *iv_normalize2_nv2i32q16_v2i32(IVNVec2D_i32q16 *a,
					       IVVec2D_i32 *b);
IVint32 iv_dist2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b);
IVint32 iv_adist2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b);
IVint32 iv_along2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b);
IVVec2D_i32 *iv_proj3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c);
IVVec2D_i32 *iv_elim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c);
IVVec2D_i32 *iv_aelim3_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVVec2D_i32 *c);
IVPoint2D_i32 *iv_proj3_p2i32_InLine_v2i32(IVPoint2D_i32 *a,
					   IVPoint2D_i32 *b,
					   IVInLine_v2i32 *c);
IVint64 iv_dist2q2_p2i32_InLine_v2i32(IVPoint2D_i32 *a, IVInLine_v2i32 *b);
IVint32 iv_dist2_p2i32_InLine_v2i32(IVPoint2D_i32 *a, IVInLine_v2i32 *b);
IVint32 iv_dist2_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVEqs_v2i32 *b);
IVint32 iv_adist2_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVEqs_v2i32 *b);
IVint32q16 iv_dist2_p2i32_Eqs_nv2i32q16(IVPoint2D_i32 *a, IVEqs_nv2i32q16 *b);
IVint32 iv_dist2_p2i32_NRay_v2i32(IVPoint2D_i32 *a, IVNLine_v2i32 *b);
IVint32 iv_adist2_p2i32_NRay_v2i32(IVPoint2D_i32 *a, IVNLine_v2i32 *b);

IVint64 iv_dist2q2_p2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b);
IVPoint2D_i32 *iv_proj3_p2i32_Eqs_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_proj3_p2i32_NLine_v2i32(IVPoint2D_i32 *a, IVPoint2D_i32 *b,
					  IVNLine_v2i32 *c);
IVPoint2D_i32 *iv_isect3_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					  IVInLine_v2i32 *b,
					  IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_p2isect4_InLine_Eqs_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVEqs_v2i32 *c,
					    IVint64 d);
IVPoint2D_i32 *iv_isect3_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					    IVInLine_v2i32 *b,
					    IVNLine_v2i32 *c);
IVPoint2D_i32 *iv_p2isect4_InLine_NLine_v2i32(IVPoint2D_i32 *a,
					      IVInLine_v2i32 *b,
					      IVNLine_v2i32 *c,
					      IVint64 d);
IVPoint2D_i32 *iv_isect3_Ray_Eqs_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
				       IVEqs_v2i32 *c);
IVPoint2D_i32 *iv_isect3_Ray_NLine_v2i32(IVPoint2D_i32 *a, IVRay_v2i32 *b,
					 IVNLine_v2i32 *c);
IVint32q16 iv_postqualfac_dot3_i32q16_v2i32(IVVec2D_i32 *a,
					    IVVec2D_i32 *b,
					    IVint64 d);
IVint32q16 iv_apostqualfac_dot3_i32q16_v2i32(IVVec2D_i32 *a,
					     IVVec2D_i32 *b,
					     IVint64 d);

IVVec2D_i32 *iv_rf2_v2i32_Eqs_v2i32(IVVec2D_i32 *a, IVEqs_v2i32 *b);
IVNLine_v2i32 *iv_rf2_NLine_Eqs_v2i32(IVNLine_v2i32 *a, IVEqs_v2i32 *b);
IVEqs_v2i32 *iv_rf2_Eqs_NLine_v2i32(IVEqs_v2i32 *a, IVNLine_v2i32 *b);
IVNLine_v2i32 *iv_rf2_NLine_InLine_v2i32(IVNLine_v2i32 *a, IVInLine_v2i32 *b);
IVInLine_v2i32 *iv_rf2_InLine_NLine_v2i32(IVInLine_v2i32 *a, IVNLine_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_Eqs_v2i32(IVPoint2D_i32 *a, IVSys2_Eqs_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_NLine_v2i32(IVPoint2D_i32 *a,
					IVSys2_NLine_v2i32 *b);
IVPoint2D_i32 *iv_solve2_s2_InLine_v2i32(IVPoint2D_i32 *a,
					 IVSys2_InLine_v2i32 *b);
IVint32q16 iv_prequalfac_i32q16_s2_Eqs_v2i32(IVSys2_Eqs_v2i32 *a);
IVint32q16 iv_aprequalfac_i32q16_s2_Eqs_v2i32(IVSys2_Eqs_v2i32 *a);

IVMatNxM_i32 *iv_mulshr4_mnxm_i32(IVMatNxM_i32 *a,
				  IVMatNxM_i32 *b, IVMatNxM_i32 *c,
				  IVuint8 q);
IVMatNxM_i32 *iv_xpose2_mnxm_i32(IVMatNxM_i32 *a, IVMatNxM_i32 *b);

IVVec2D_i32 *iv_mean2_v2i32_arn_v2i32(IVPoint2D_i32 *a,
				      IVPoint2D_i32_array *data);
IVSys2_Eqs_v2i32q16 *iv_pack_linreg3_s2_Eqs_v2i32q16_arn_v2i32
  (IVSys2_Eqs_v2i32q16 *sys, IVPoint2D_i32_array *data, IVuint8 q);

/********************************************************************/

IVVec3D_i32 *iv_neg2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b);
IVVec3D_i32 *iv_add3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c);
IVVec3D_i32 *iv_sub3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c);
IVVec3D_i32 *iv_muldiv4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint32 c, IVint32 d);
IVVec3D_i32 *iv_muldiv4_v3i32_i64(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint64 c, IVint64 d);
IVVec3D_i32 *iv_muldiv4_v3i32_i64_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				      IVint64 c, IVint32 d);
IVVec3D_i32 *iv_shldiv4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVuint8 c, IVint32 d);
IVVec3D_i32 *iv_mulshr4_v3i32_i32(IVVec3D_i32 *a, IVVec3D_i32 *b,
				  IVint32 c, IVuint8 q);
IVVec3D_i32 *iv_shl3_v3i32_u32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 c);
IVVec3D_i32 *iv_shr3_v3i32_u32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 c);
IVint64 iv_dot2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b);
IVint64 iv_magn2q_v3i32(IVVec3D_i32 *a);
IVVec3D_i32 *iv_crossproddiv4_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c,
				    IVint32 d);
IVVec3D_i32 *iv_crossprodshr4_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c,
				    IVuint8 q);
IVuint8 iv_cnf_crossprod2_v3i32(IVVec3D_i32 *b, IVVec3D_i32 *c);
IVVec3D_i32 *iv_crossprod3_cn_v3i32(IVVec3D_i32 *a,
				    IVVec3D_i32 *b, IVVec3D_i32 *c);
IVint32q16 iv_postqualfac_cps4_i32q16_v3i32
  (IVVec3D_i32 *c, IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q);
IVint32q16 iv_apostqualfac_cps4_i32q16_v3i32
  (IVVec3D_i32 *c, IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q);
IVVec3D_i32 *iv_nosol_v3i32(IVVec3D_i32 *a);
IVuint8 iv_is_nosol_v3i32(IVVec3D_i32 *a);
IVuint8 iv_is_zero_v3i32(IVVec3D_i32 *a);

IVint32 iv_magnitude_v3i32(IVVec3D_i32 *a);
IVint32 iv_magn_v3i32(IVVec3D_i32 *a);
IVint32 iv_magn_log2_v3i32(IVVec3D_i32 *a);
IVuint8 iv_amagn_log2_v3i32(IVVec3D_i32 *a);
IVNVec3D_i32q16 *iv_normalize2_nv3i32q16_v3i32(IVNVec3D_i32q16 *a,
					       IVVec3D_i32 *b);
IVint32 iv_dist2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b);
IVint32 iv_adist2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b);
IVint32 iv_along2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b);
IVVec3D_i32 *iv_proj3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c);
IVVec3D_i32 *iv_elim3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c);
IVVec3D_i32 *iv_aelim3_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVVec3D_i32 *c);
IVint32 iv_dist2_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVEqs_v3i32 *b);
IVint32 iv_adist2_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVEqs_v3i32 *b);
IVint32q16 iv_dist2_p3i32_Eqs_nv3i32q16(IVPoint3D_i32 *a, IVEqs_nv3i32q16 *b);
IVint32 iv_dist2_p3i32_NRay_v3i32(IVPoint3D_i32 *a, IVNPlane_v3i32 *b);
IVint32 iv_adist2_p3i32_NRay_v3i32(IVPoint3D_i32 *a, IVNPlane_v3i32 *b);

IVint64 iv_dist2q2_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b);
IVPoint3D_i32 *iv_proj3_p3i32_Eqs_v3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					IVEqs_v3i32 *c);
IVPoint3D_i32 *iv_proj3_p3i32_NPlane_v3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					   IVNPlane_v3i32 *c);
IVPoint3D_i32 *iv_isect3_InLine_Eqs_v3i32(IVPoint3D_i32 *a,
					  IVInLine_v3i32 *b,
					  IVEqs_v3i32 *c);
IVPoint3D_i32 *iv_p2isect4_InLine_Eqs_v3i32(IVPoint3D_i32 *a,
					    IVInLine_v3i32 *b,
					    IVEqs_v3i32 *c,
					    IVint64 d);
IVPoint3D_i32 *iv_isect3_InLine_NPlane_v3i32(IVPoint3D_i32 *a,
					     IVInLine_v3i32 *b,
					     IVNPlane_v3i32 *c);
IVPoint3D_i32 *iv_p2isect4_InLine_NPlane_v3i32(IVPoint3D_i32 *a,
					       IVInLine_v3i32 *b,
					       IVNPlane_v3i32 *c,
					       IVint64 d);
IVPoint3D_i32 *iv_isect3_Ray_Eqs_v3i32(IVPoint3D_i32 *a, IVRay_v3i32 *b,
				       IVEqs_v3i32 *c);
IVPoint3D_i32 *iv_isect3_Ray_NPlane_v3i32(IVPoint3D_i32 *a, IVRay_v3i32 *b,
					  IVNPlane_v3i32 *c);
IVint32q16 iv_postqualfac_dot3_i32q16_v3i32(IVVec3D_i32 *a,
					    IVVec3D_i32 *b,
					    IVint64 d);
IVint32q16 iv_apostqualfac_dot3_i32q16_v3i32(IVVec3D_i32 *a,
					     IVVec3D_i32 *b,
					     IVint64 d);

IVVec3D_i32 *iv_rf2_v3i32_Eqs_v3i32(IVVec3D_i32 *a, IVEqs_v3i32 *b);
IVNPlane_v3i32 *iv_rf2_NPlane_Eqs_v3i32(IVNPlane_v3i32 *a, IVEqs_v3i32 *b);
IVEqs_v3i32 *iv_rf2_Eqs_NPlane_v3i32(IVEqs_v3i32 *a, IVNPlane_v3i32 *b);
IVNPlane_v3i32 *iv_rf2_NPlane_InPlane_v3i32(IVNPlane_v3i32 *a,
					    IVInPlane_v3i32 *b);
IVInPlane_v3i32 *iv_rfh4_InPlane_NPlane_v3i32
  (IVInPlane_v3i32 *a, IVNPlane_v3i32 *b, IVVec3D_i32 *h0, IVVec3D_i32 *h1);
IVInPlane_v3i32 *iv_rfbh4_InPlane_NPlane_v3i32
  (IVInPlane_v3i32 *a, IVNPlane_v3i32 *b, IVVec3D_i32 *h0, IVVec3D_i32 *h1);
IVPoint3D_i32 *iv_solve2_s3_Eqs_v3i32(IVPoint3D_i32 *a, IVSys3_Eqs_v3i32 *b);
IVPoint3D_i32 *iv_solve2_s3_NPlane_v3i32(IVPoint3D_i32 *a,
					 IVSys3_NPlane_v3i32 *b);
IVPoint3D_i32 *iv_solve2_s3_InPlane_v3i32(IVPoint3D_i32 *a,
					  IVSys3_InPlane_v3i32 *b);
IVint32q16 iv_prequalfac_i32q16_s3_Eqs_v3i32(IVSys3_Eqs_v3i32 *a);
IVint32q16 iv_aprequalfac_i32q16_s3_Eqs_v3i32(IVSys3_Eqs_v3i32 *a);

IVVec3D_i32 *iv_mean2_v3i32_arn_v3i32(IVPoint3D_i32 *a,
				      IVPoint3D_i32_array *data);
IVSys3_Eqs_v3i32q16 *iv_pack_linreg3_s3_Eqs_v3i32q16_arn_v3i32
  (IVSys3_Eqs_v3i32q16 *sys, IVPoint2D_i32_array *data, IVuint8 q);

IVuint64 iv_triarea2_v2i32(IVVec2D_i32 *a, IVVec2D_i32 *b, IVuint8 q);
IVuint64 iv_triarea2_v3i32(IVVec3D_i32 *a, IVVec3D_i32 *b, IVuint8 q);
IVuint64 iv_pyrvol2_u64_u32(IVuint64 a, IVuint32 b);

IVPoint3D_i32 *iv_w2v4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
			     IVRect_p2i32 *world, IVRect_p2i32 *viewport);
IVPoint3D_i32 *iv_w2v_shr4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				 IVRect_p2i32 *world,
				 IVRect_p2i32 *viewport);
IVPoint3D_i32 *iv_w2v_eshr4_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				  IVRect_p2i32 *world,
				  IVRect_p2i32 *viewport);
IVPoint3D_i32 *iv_persproj3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				  IVint32 focus);
IVPoint3D_i32 *iv_unpersproj3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				    IVint32 focus);
IVPoint3D_i32 *iv_persproj_homo3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
				       IVint32 focus, IVuint8 q);
IVPoint3D_i32 *iv_unpersproj_homo3_p3i32(IVPoint3D_i32 *a, IVPoint3D_i32 *b,
					 IVint32 focus, IVuint8 q);

#endif /* not IVECMATH_H */
