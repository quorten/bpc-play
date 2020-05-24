/* Rational, fractional, quotient-remainder math with integers.

   Design note: Typically, in serious applications, whenever you need
   to work with fractions, you'll want to convert them all to a common
   denominator right away using a multiply-divide (or shift-divide)
   operation.  Therefore, it is typically not necessary to store
   denominator information in the data structures for individual
   numbers.  There is a very special case which is extremely useful,
   the improper fraction format with a power-of-two common
   denominator, otherwise known as fixed-point numbers.  Floating
   point numbers are a close variant that instead require a common
   denominator factor (powers of two) rather than a common denominator
   itself.  Both cases allow the use of the more efficient
   multiply-shift operation rather than multiply-divide for many
   operations.

   Because of these reasons, there are only a few cases where
   fractional arithmetic subroutines are truly useful in serious
   applications.

   Conversely, because power-of-two fractional arithmetic is extremely
   useful in serious applications, a considerable portion of the
   fractional arithmetic subroutines are devoted to handling those
   particular cases.  */

/* Proper fraction format with known common denominator stored
   elsewhere.  Suitable for a wide range of remainder-quotient uses in
   measurement units like bits and bytes, inches and feet, etc.  */
struct IFRemQuot_u8_u32_tag
{
  unsigned char n; /* remainder, i.e. numerator */
  unsigned int i; /* quotient, i.e. integral part */
};
typedef struct IFRemQuot_u8_u32_tag IFRemQuot_u8_u32;

struct IFRemQuot_i8_i32_tag
{
  signed char n; /* remainder, i.e. numerator */
  int i; /* quotient, i.e. integral part */
};
typedef struct IFRemQuot_i8_i32_tag IFRemQuot_i8_i32;

struct IFRemQuot_i16_tag
{
  short n; /* remainder, i.e. numerator */
  short i; /* quotient, i.e. integral part */
};
typedef struct IFRemQuot_i16_tag IFRemQuot_i16;

struct IFRemQuot_i32_tag
{
  int n; /* remainder, i.e. numerator */
  int i; /* quotient, i.e. integral part */
};
typedef struct IFRemQuot_i32_tag IFRemQuot_i32;

/* Improper fraction format, best suited for packing operands before
   calling multiply-divide operators.  */
struct IFMulDiv_i16_tag
{
  short n; /* numerator */
  short d; /* denominator */
};
typedef struct IFMulDiv_i16_tag IFMulDiv_i16;

struct IFMulDiv_i32_tag
{
  int n; /* numerator */
  int d; /* denominator */
};
typedef struct IFMulDiv_i32_tag IFMulDiv_i32;

struct IFMulDiv_i64_tag
{
  long long n; /* numerator */
  long long d; /* denominator */
};
typedef struct IFMulDiv_i64_tag IFMulDiv_i64;

/* Proper fraction format with denominator.  Probably the main
   application use here is if the data comes in as this format, this
   data structure provides a convenient way to pack it for format
   conversions.  */
struct IFPropFrac_i32_tag
{
  int n; /* remainder, i.e. numerator */
  int d; /* denominator */
  int i; /* quotient, i.e. integral part */
};
typedef struct IFPropFrac_i32_tag IFPropFrac_i32;
