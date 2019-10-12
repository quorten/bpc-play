/* Miniature system call definitions for x86-64 Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_64.h"
#include "syscalls.h"

extern int errno;

long
write (int fd, const void *buf, unsigned long count)
{
  long result;
  INTERNAL_SYSCALL_3(__NR_write, , result, fd, buf, count);
  if (result < 0)
    { errno = (int)(-result); result = -1; }
  else
    errno = 0;
  return result;
}
