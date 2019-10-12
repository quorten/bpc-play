/* Miniature system call definitions for x86 32-bit Linux.  Note that
   this will also work on x86-64 Linux systems that support the x32
   ABI.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"

extern int errno;

void
_exit (int status)
{
  INTERNAL_SYSCALL_EXIT(__NR_exit, status);
}
