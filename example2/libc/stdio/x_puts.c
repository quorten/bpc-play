/* See the COPYING file for license details.  */

#include "private.h"

int
x_puts (const char *str)
{
  while (*str) {
    if (putchar (*str++) == EOF)
      return EOF;
  }
  return 0;
}
