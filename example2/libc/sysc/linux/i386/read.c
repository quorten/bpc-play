/* Miniature system call definitions for x86 32-bit Linux.  Note that
   this will also work on x86-64 Linux systems that support the x32
   ABI.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"

extern int errno;

long
read (int fd, void *buf, unsigned long count)
{
  long result;
  INTERNAL_SYSCALL_3(__NR_read, , result, fd, buf, count);
  if (result < 0)
    { errno = -result; result = -1; }
  else
    errno = 0;
  return result;
}
