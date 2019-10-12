/* Miniature system call definitions for ARM EABI Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"

extern int errno;

long
write (int fd, const void *buf, unsigned long count)
{
  long result;
  INTERNAL_SYSCALL_3(__NR_write, , __r0, fd, buf, count);
  if (__r0 >= (unsigned long)-4095)
    { errno = (int)-__r0; result = -1; }
  else
    { errno = 0; result = __r0; }
  return result;
}
