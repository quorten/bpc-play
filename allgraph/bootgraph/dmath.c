/* Dumb math, integer multiply and divide subroutines for processors
   that lack them.  Reference implementation written in C, of course
   you'll want to rewrite in assembly language for your target
   architecture.  */

#include <stdio.h>

struct QuotRem16_tag
{
  unsigned short q; /* quotient */
  unsigned short r; /* remainder */
};
typedef struct QuotRem16_tag QuotRem16;

struct IQuotRem16_tag
{
  short q; /* quotient */
  short r; /* remainder */
};
typedef struct IQuotRem16_tag IQuotRem16;

/* 16-bit unsigned integer multiply */
unsigned int mul16 (unsigned short a, unsigned short b)
{
  unsigned int work_a = a;
  unsigned int p = 0;
  while (b != 0) {
    if ((b & 1) != 0)
      p += work_a;
    work_a <<= 1;
    b >>= 1;
  }
  return p;
}

/* Clever swap without temporary variable but bad compilers can
   generate bugs.  */
#define SWAP(a, b) ((a) ^= (b) ^= (a) ^= (b))

/* 16-bit signed integer multiply */
int imul16 (short a, short b)
{
  int work_a;
  int p = 0;
  if (a > 0 && b > 0)
    return mul16 (a, b);
  if (b > 0)
    SWAP(a, b);
  work_a = a;
  /* Treat the first iteration specially, to handle the fact that
     negatives have a larger range than positives.  */
  if ((b & 1) != 0) {
    p -= work_a;
    b >>= 1; b++;
  } else
    b >>= 1;
  work_a <<= 1;
  /* Now we can just negate and treat the rest almost the same as the
     positive case.  */
  b = -b;
  while (b != 0) {
    if ((b & 1) != 0)
      p -= work_a;
    work_a <<= 1;
    b >>= 1;
  }
  return p;
}

/* 16-bit unsigned integer divide.  If `b == 0` then we act silly and
   set the quotient to UINT16_MAX and the remainder to `a`.

   N.B.: Checking for leading zeros and skipping accordingly is faster
   than this default, worst-case approach.  There are also several
   other cases we can check for for faster special case and overall
   performance.  */
QuotRem16 *div16 (QuotRem16 *qr, unsigned short a, unsigned short b)
{
  unsigned short q = 0;
  unsigned short q_test = 0x8000;
  unsigned short work_r = a;
  unsigned int work_b = b << 0x10;
  while (q_test != 0) {
    work_b >>= 1;
    if ((unsigned int)work_r >= work_b) {
      q |= q_test;
      work_r -= work_b;
    }
    q_test >>= 1;
  }
  qr->q = q;
  qr->r = work_r;
  return qr;
}

/* 16-bit signed integer divide.  If `b == 0` then we act silly and
   set the quotient to UINT16_MAX and the remainder to `a`.  */
IQuotRem16 *idiv16 (IQuotRem16 *qr, short a, short b)
{
  unsigned char negated = 0;
  unsigned short q = 0;
  unsigned short q_test = 0x8000;
  short work_r;
  int work_b;

  if (a > 0 && b > 0)
    return (IQuotRem16*)div16 ((QuotRem16*)qr, a, b);
  /* Note that due to the constraints division imposes, we can safely
     negate under this condition test and get the same results without
     needing to worry about INT_MIN overflowing INT_MAX.  */
  if (b > 0)
    { negated = 1; a = -a; b = -b; }
  /* NOTE: If we have (a < 0 && b < 0), we cannot negate and use
     unsigned integer division as-is since (-INT_MIN == INT_MIN).
     Therefore, we handle division by two negatives separately.  */
  work_r = a;
  work_b = b << 0x10;

  if (a < 0 && b < 0) {
    while (q_test != 0) {
      work_b >>= 1;
      if ((int)work_r <= work_b) {
	q |= q_test;
	work_r -= work_b;
      }
      q_test >>= 1;
    }
  } else { /* (a > 0 && b < 0) */
    while (q_test != 0) {
      work_b >>= 1;
      if ((int)work_r + work_b >= 0) {
	q |= q_test;
	work_r += work_b;
      }
      q_test >>= 1;
    }
    /* Now negate `q` since we should have a negative quotient when
       the signs differ.  */
      q = -q;
      /* Negate the remainder if we were using negatives in our
       intermediate computation.  */
      if (negated) work_r = -work_r;
  }

  qr->q = q;
  qr->r = work_r;
  return qr;
}

/* 16-bit signed integer divide.  If `b == 0` then we act silly and
   set the quotient to UINT16_MAX and the remainder to `a`.  "Safe
   version" handles all sign cases separately rather than trying to
   merge code together.  */
IQuotRem16 *safe_idiv16 (IQuotRem16 *qr, short a, short b)
{
  unsigned short q = 0;
  unsigned short q_test = 0x8000;
  short work_r;
  int work_b;

  if (a > 0 && b > 0)
    return (IQuotRem16*)div16 ((QuotRem16*)qr, a, b);
  work_r = a;
  work_b = b << 0x10;

  if (a < 0 && b < 0) {
    while (q_test != 0) {
      work_b >>= 1;
      if ((int)work_r <= work_b) {
	q |= q_test;
	work_r -= work_b;
      }
      q_test >>= 1;
    }
  } else if (a > 0 && b < 0) {
    while (q_test != 0) {
      work_b >>= 1;
      if ((int)work_r + work_b >= 0) {
	q |= q_test;
	work_r += work_b;
      }
      q_test >>= 1;
    }
    /* Now negate `q` since we should have a negative quotient when
       the signs differ.  */
    q = -q;
  } else { /* (a < 0 && b > 0) */
    while (q_test != 0) {
      work_b >>= 1;
      if ((int)work_r + work_b <= 0) {
	q |= q_test;
	work_r += work_b;
      }
      q_test >>= 1;
    }
    /* Now negate `q` since we should have a negative quotient when
       the signs differ.  */
    q = -q;
  }
  qr->q = q;
  qr->r = work_r;
  return qr;
}

int
main (void)
{
  unsigned short a = 5, b = 7;
  short ia = -3, ib = -5;
  unsigned short n = 5, d = 3;
  QuotRem16 qr;
  IQuotRem16 iqr;
  printf ("%d * %d == %d\n", a, b, mul16 (a, b));
  printf ("%d * %d == %d\n", ia, ib, imul16 (ia, ib));
  ia = -5; ib = 3;
  printf ("%d * %d == %d\n", ia, ib, imul16 (ia, ib));
  ia = 5; ib = -3;
  printf ("%d * %d == %d\n", ia, ib, imul16 (ia, ib));
  ia = 5; ib = 3;
  printf ("%d * %d == %d\n", ia, ib, imul16 (ia, ib));
  div16 (&qr, n, d);
  printf ("%d / %d == %d + %d / %d\n", n, d, qr.q, qr.r, d);
  ia = -5; ib = -2;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  ia = -5; ib = 4;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  ia = 5; ib = -4;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);

  ia = -35; ib = -7;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  ia = -35; ib = 7;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  n = 35; d = 7;
  div16 (&qr, n, d);
  printf ("%d / %d == %d + %d / %d\n", n, d, qr.q, qr.r, d);

  ia = -32768; ib = 1;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  ia = -32768; ib = -1;
  idiv16 (&iqr, ia, ib);
  printf ("%d / %d == %d + %d / %d\n", ia, ib, iqr.q, iqr.r, ib);
  ib = -ia;
  printf ("-(%d) == %d\n", ia, ib);

  puts ("Be careful with fast bit-wise math idioms on signed integers!");
  puts ("Consider the following examples:");
  printf ("3 * 2 = 3 << 1 == %d\n", 3 << 1);
  printf ("-3 << 1 == %d\n", -3 << 1);
  printf ("7 / 2 == 7 >> 1 == %d\n", 7 >> 1);
  printf ("-7 >> 1 == %d\n", -7 >> 1);
  printf ("30 / 4 == 30 >> 2 == %d\n", 30 >> 2);
  printf ("-30 >> 2 == %d\n", -30 >> 2);
  printf ("3 rem 4 == 3 & (4 - 1) == %d\n", 3 & (4 - 1));
  printf ("-3 & (4 - 1) == %d\n", -3 & (4 - 1));
  printf ("(27 quot 4) * 4 == 27 & ~(4 - 1) == %d\n", 27 & ~(4 - 1));
  printf ("-27 & ~(4 - 1) == %d\n", -27 & ~(4 - 1));
  puts ("");

  printf ("-7 / 2 == (-7 + 1) >> 1 == %d\n", (-7 + 1) >> 1);
  printf ("-30 / 4 == (-30 + ((1 << 2) - 1)) >> 2 == %d\n", (-30 + ((1 << 2) - 1)) >> 2);
  printf ("(-27 quot 4) * 4 == (-27 + (4 - 1)) & ~(4 - 1) == %d\n", (-27 + (4 - 1)) & ~(4 - 1));
  printf ("(-28 quot 4) * 4 == (-28 + (4 - 1)) & ~(4 - 1) == %d\n", (-28 + (4 - 1)) & ~(4 - 1));
  printf ("(-28 quot 4) * 4 != (-28 + 4) & ~(4 - 1) == %d\n", (-28 + 4) & ~(4 - 1));

  return 0;
}
