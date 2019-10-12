/* See the COPYING file for license details.  */

#include "private.h"

int
puts (const char *str)
{
  if (x_puts (str) == EOF)
    return EOF;
  if (putchar ('\n') == EOF)
    return EOF;
  return 0;
}
