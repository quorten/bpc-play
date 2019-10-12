/* Miniature system call definitions for x86 32-bit Linux.  Note that
   this will also work on x86-64 Linux systems that support the x32
   ABI.  Currently, only `gcc' compiler-syntax is defined here.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"

void
_exit (int status)
{
  syscall1(__NR_exit, status);
}
