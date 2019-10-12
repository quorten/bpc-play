/* See the COPYING file for license details.  */

#include "private.h"

int
getchar (void)
{
  static unsigned char buf[128];
  static unsigned int buf_sz = 0;
  static unsigned int buf_pos = 0;
  if (buf_pos < buf_sz)
    return buf[buf_pos++];
  /* else Fill the input buffer.  */
  buf_pos = 0;
  buf_sz = (unsigned int) read (STDIN_FILENO, buf, 128);
  if (buf_sz == -1 || buf_sz == 0)
    return EOF;
  return buf[buf_pos++];
}
