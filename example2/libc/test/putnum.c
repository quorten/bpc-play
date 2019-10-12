/* See the COPYING file for license details.  */

#include <stdio.h>

#define MAX_DIGITS (10)
typedef unsigned char DigitsIdx;

#define MAX_BITS (sizeof(int) << 3) /* (sizeof(int) * 8) */
typedef unsigned char BitsIdx;

/* NOTE: We use a BCD addition technique to avoid using multiplication
   or division to convert a number from binary to decimal.  Some early
   microprocessors did not have multiplication or division
   instructions.  Plus, avoiding multiplication and division generally
   results in faster code.  */
void
putnum (int num)
{
#define BCD_ADD(n1, n2) \
  ({ \
    unsigned char carry = 0; \
    i = MAX_DIGITS; \
    do { \
      i--; \
      n1[i] += n2[i]; \
      if (carry) \
        { n1[i]++; carry = 0; } \
      if (n1[i] >= 10) { \
        n1[i] -= 10; \
        carry = 1; \
      } \
    } while (i > 0); \
  })

  unsigned char outbuf[MAX_DIGITS];
  unsigned char addbuf[MAX_DIGITS];
  register DigitsIdx i = 0;
  BitsIdx j = 0;
  if (num < 0) {
    putchar ('-');
    num = -num;
  }
  while (i < MAX_DIGITS) {
    outbuf[i] = 0;
    addbuf[i] = 0;
    i++;
  }
  addbuf[MAX_DIGITS-1] = 1;
  while (j < MAX_BITS) {
    if ((num & 1))
      BCD_ADD(outbuf, addbuf);
    num >>= 1;

    /* addbuf *= 2 */
    BCD_ADD(addbuf, addbuf);
    j++;
  }
  i = 0;
  /* Skip leading zeros.  */
  while (i < MAX_DIGITS - 1 && outbuf[i] == 0)
    i++;
  while (i < MAX_DIGITS)
    putchar (0x30 + outbuf[i++]);

#undef BCD_ADD
}
