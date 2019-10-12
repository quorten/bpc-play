/* See the COPYING file for license details.  */

#include "private.h"

long
write_all (int fd, const void *buf, unsigned long count)
{
  const unsigned char *cur_pos = buf;
  unsigned long num_pending = count;
  do {
    unsigned long result = write (fd, cur_pos, num_pending);
    if (result == -1) {
      if (errno == EINTR)
        continue;
      else
        return -1;
    }
    cur_pos += result;
    num_pending -= result;
  } while (num_pending > 0);
  return count;
}
