/* See the COPYING file for license details.  */

#include "private.h"

long write_all (int fd, const void *buf, unsigned long count);

int
putchar (int c)
{
  static unsigned char buf[128];
  static unsigned int buf_sz = 0;
  buf[buf_sz++] = (unsigned char)c;
  if (c != '\n' && buf_sz < 128)
    return c;
  /* else Flush the output buffer.  */
  if (write_all (STDOUT_FILENO, buf, buf_sz) == -1)
    return EOF;
  buf_sz = 0;
  return c;
}
