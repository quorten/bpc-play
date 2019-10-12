/* Miniature system call definitions for x86 32-bit Linux.  Note that
   this will also work on x86-64 Linux systems that support the x32
   ABI.  Currently, only `gcc' compiler-syntax is defined here.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"

long
read (int fd, void *buf, unsigned long count)
{
  return syscall3(__NR_read, fd, buf, count);
}
