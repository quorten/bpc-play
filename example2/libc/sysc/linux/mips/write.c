/* Miniature system call definitions for MIPS32 Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"

extern int errno;

long
write (int fd, const void *buf, unsigned long count)
{
  long result;
  INTERNAL_SYSCALL_3(__NR_write, __a3, __v0, fd, buf, count);
  if (__a3 != 0)
    { errno = (int)__v0; result = -1; }
  else
    { errno = 0; result = __v0; }
  return result;
}
