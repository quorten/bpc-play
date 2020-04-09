/* Abstract data type math library, in C.

   How does this work?  Mathematical routines are defined on an
   abstract data typee.  A simple `sed' script then replaces that
   abstract data type with a specific data type, and writes the result
   out to a file.  Functions typically have the abstract data type
   within their names so that multiple such compilation units can be
   linked in.

*/

/* Abstract table of contents:

Two main variants for substitution:

abnum = abstract number
brnum = abbreviated symbol for abstract number, for use in type names,
        functions, etc.

Level 1:
abidx
bridx
abnum
brnum

Level 2:
abvec
brvec
abpoint
brpoint

*/

/* TODO: Convert fixarray to exparray, and so on.  */

/* Fully fixed array.  Both the allocation and the length of the array
   are fixed and identical.  This is mainly some helper macros to make
   generics programming easier.  */
#define FFA_TYPE(typename, length) \
  struct typename##_ar##length##_tag \
  { \
    typename d[length]; \
  }; \
  typedef struct typename##_ar##length##_tag typename##_ar##length

#define FFA_LEN(array) \
  (sizeof((array).d) / sizeof((array).d[0]))

FFA_TYPE(abnum, 2);
FFA_TYPE(abnum, 3);
FFA_TYPE(abnum, 4);
FFA_TYPE(abnum, 9);
FFA_TYPE(abnum, 16);
/* Abstract {2,3,4}-dimensional vectors and points */
typedef abnum_ar2 AMVec2D_brnum;
typedef abnum_ar2 AMPoint2D_brnum;
typedef abnum_ar3 AMVec3D_brnum;
typedef abnum_ar3 AMPoint3D_brnum;
typedef abnum_ar4 AMVec4D_brnum;
typedef abnum_ar4 AMPoint4D_brnum;
/* Abstract {2,3,4}x{2,3,4} matrices */
typedef abnum_ar4 AMMat2x2_brnum;
typedef abnum_ar9 AMMat3x3_brnum;
typedef abnum_ar16 AMMat4x4_brnum;
/* Abstract {2,3,4}x{2,3,4} transposed matrices */
typedef abnum_ar4 AMTMat2x2_brnum;
typedef abnum_ar9 AMTMat3x3_brnum;
typedef abnum_ar16 AMTMat4x4_brnum;

EA_TYPE(abnum);
/* Abstract n-dimensional vectors and points */
typedef abnum_array AMVecND_brnum;
typedef abnum_array AMPointND_brnum;
/* Abstract nxn matrices and transposed matrices */
struct AMMatNxN_brnum_tag
{
  abnum *d;
  unsigned short width;
  unsigned short pitch; /* Size in bytes of a row */
  unsigned short height;
};
typedef struct AMMatNxN_brnum_tag AMMatNxN_brnum;
typedef AMMatNxN_brnum AMTMatNxN_brnum;

FFA_TYPE(abpoint, 2);
FFA_TYPE(abpoint, 3);
/* Abstract line segment */
typedef abpoint_ar2 AMLineSeg_brpoint;
/* Abstract triangle */
typedef abpoint_ar3 AMTri_brpoint;

FFA_TYPE(abidx, 2);
FFA_TYPE(abidx, 3);
/* Abstract line segment from indices */
typedef abidx_ar2 AMLineSeg_bridx;
/* Abstract Triangle from Indices */
typedef abidx_ar3 AMTri_bridx;

/* Abstract simple linear equation */
struct Eqs_brvec_tag
{
  abvec v; /* "left" side of vector equation */
  abnum offset; /* "right" side of vector equation */
};
typedef struct Eqs_brvec_tag Eqs_brvec;

/* N.B. An abstract simple linear equation can also function as the
   equation of a line, the equation of a plane, etc.  */

/* Abstract ray: [0] direction vector, [1] ray origin ("point zero") */
struct Ray_brvec_tag
{
  abvec v; /* "left" side of vector equation */
  abpoint p0; /* "right" side of vector equation */
};
typedef struct Ray_brvec_tag Ray_brvec;

/* N.B. Note that the abstract ray data structure can also function as
   a line equation, a plane equation, and therefore even an equation
   in a system of linear equations.  */

/* Abstract dimensional system of {2,3,4} equations */
FFA_TYPE(Eqs_brvec, 2);
FFA_TYPE(Eqs_brvec, 3);
FFA_TYPE(Eqs_brvec, 4);
typedef Eqs_brvec_ar2 AMSys2_Eqs_brvec;
typedef Eqs_brvec_ar3 AMSys3_Eqs_brvec;
typedef Eqs_brvec_ar4 AMSys4_Eqs_brvec;
FFA_TYPE(Ray_brvec, 2);
FFA_TYPE(Ray_brvec, 3);
FFA_TYPE(Ray_brvec, 4);
typedef Ray_brvec_ar2 AMSys2_Ray_brvec;
typedef Ray_brvec_ar3 AMSys3_Ray_brvec;
typedef Ray_brvec_ar4 AMSys4_Ray_brvec;

/* Common arbitrary length array types */
/* Point cloud */
EA_TYPE(abpoint);
EA_TYPE(AMLineSeg_brpoint);
EA_TYPE(AMLineSeg_bridx);
/* Triangle mesh/surface */
EA_TYPE(AMTri_brpoint);
/* Triangle mesh/surface from indices */
EA_TYPE(AMTri_bridx);
/* n-length "over-constrained" system of equations */
EA_TYPE(Eqs_brvec);
EA_TYPE(Ray_brvec);

/* TODO: Add rational numbers too.  */

/* TODO: Support saturating arithmetic.  */

/* TODO: Review this and try to use abbreviations whenever possible.
   No worries, these an be standardized.

   unsigned integer = u, signed integer = s, unsigned fixed point =
   uq, signed fixed point = q, float = f, double = d, long double = ld

   "Real number" data type: s8, s16, s32, s64, q8, q16, q32, q1_15, f,
   d, ld
   Dimension: 1, 2, 3, 4
   Vector: v
   Point: pt

   So, this makes worst case read as follows:
   AMSys4_Ray_v4s32

   Wow, now that's a lot better.

*/

/********************************************************************/
/* ???  These are typically unnecessary unless you want the
   possibility to swap an arbitrary precision library in place.
   Actually, these are very useful for, say, fixed point arithmetic.
   This is good, because you can simply eliminate these definitions
   and replace them with your own, and the generics will target the
   correct code.  Use force inline judiciously.  */
abnum
am_add_brnum (abnum a, abnum b)
{
  return a + b;
}

abnum
am_sub_brnum (abnum a, abnum b)
{
  return a - b;
}

abnum
am_mul_brnum (abnum a, abnum b)
{
  return a * b;
}

abnum
am_div_brnum (abnum a, abnum b)
{
  return a / b;
}

/* multiply by power of two */

/* divide by power of two */

/* square a number */

/* cube a number */

/* TODO: Approximate squares and approximate square roots.  */

/* TODO: Approximate cube and cube root */

/* TODO: Compute square root of two as a fixed point number.  */

/* TODO: Compute number e as a fixed point number.  */

/* TODO: Generics support for symbols and expressions.  After all, it
   is required by a basic C compiler.  The concept is simple:
   variables are ID numbers, numbers are, well, also numbers.  A
   symbol table is used to translate a variable ID number to a string,
   memory address, and so on.  Some symbols are reserved for
   operators, the rest can be used for variables, functions, and so
   on.  */

typedef enum { false, true } bool;

struct ABToken_brnum_tag
{
  union {
    abnum num;
    abidx sym;
  } t;
  unsigned char is_sym : 1; /* 0 if number, 1 if symbol */
};
typedef struct struct ABToken_brnum_tag ABToken_brnum;

/* So now we can represent expressions for computer algebra,
   canonicalize expressions, then solve equations.  */

/* TODO: Sieve of Erathothenes, Sieve of Atkin, factoring, GCF,
   LCD.  */

/* TODO: Great idea?  Carry and overflow?  These are data structure
   members of the built-in number type.  Or alternatively, there are
   accessor functions for these members.  Depending on the code of the
   flow, these values may never need to be persisted for a long period
   of time.  But yes, it totally is possible to access this
   information with good-style C code.  Heck, you can program it
   yourself using bit-fields if you don't mind loosing one bit off of
   your numbers.  Only for carry.  In the basic case, only effective
   for unsigned integer arithmetic.  That idea works okay, actually.
   Only signed positive integers are allowed.  */

/* TODO: Monte Carlo methods, Halton series, adaptive Monte Carlo
   methods, Quasi-Monte-Carlo methods, adaptive QMC.  */

/* Done!  Type tags defined in the manner of embedded linked list.
   We've got both cases covered with our `navrec.h' macro library.  */

/* Technically, ABToken is not math, it's for symbols support.  Symbol
   support should go in a separate header file.  Likewise with
   higher-level vector operations.  Heck, ABToken IS a special case of
   tagged data, you may just as well want to extend to support more
   types.  The main unknown is how many types you want to support,
   which defines the size of the fields.  */

/********************************************************************/
/* Now, for generatized vector functions!  */

void vec_add (void);
void vec_sub (void);
/* Scalar multiplication */
void vec_s_mul (void);

/* magnitude, "length" is dimensionality due to the fact we build on
   top of generalized array abstractions.  */

/* Intersect ray on plane */
/* Find perpendicular point on plane */

/* dot product */
/* cross product */

/* distance vector approximate/exact */
/* distance squared vector approximate/exact */

/* Solve system of equations, vector-point method.  Fully general.  */
/* Solve system of equations, Gaussian elimination with partial
   pivoting, row-reduced echelon form.  Does not currently work on
   integer arithmetic.  */

/* echelon form, triangular form */
/* reduced echelon form */
/* Row-reduced echelon form, use partial pivoting  */

/* Compute linear regresssions, generics functions */

/* TODO: Multi-dimensional single-stepping, i.e. line plotter.  */
/* TODO: Quadratic bezier single-stepping.  */
/* TODO: Circle plotting algorithm, compute trigonometric lookup
   tables in fixed point.  */

/* TODO: Trigonometric functions, lookup table.  */
/* TODO: Trigonometric functions, Mcclaurin series.  */

/* square, e, cube, power of 4 */
/* exponent and lookup tables */
/* quadratic formula solution, positive only, positive and negative */
/* gamma curves, lookup tables, bi-directional conversions.  with
   common base integer arithmetic */
/* common gamma curves, 1.8, 2.2 */


/* TODO: Statistical functions */
/* mean */
/* median */
/* mode */
/* range */
/* standard deviation */
/* r^2 */

/* statistical functions common in raytracers */
/* pdf probability density function */
/* cdf cumulative density function */

/********************************************************************/
/* Application-specific math functions */

/* bit array functions */

/* pixel image buffer functions */

/* TODO: Recursive mesh geometry rendering trees */

/* TODO: Catmull-Clark subdivision */

/* TODO: Triangular NURBS?  */

/* TODO: Low color depth bitmap compressors.  RLE (continuous color),
   Lempel-Ziv (repeating dither patterns), reverse error diffusion
   (lossy, probabilistic, and possible addition to use DCT
   analysis).  */

/* TODO: Recursive  mesh partitioning */

/* TODO: Boolean geometry */
