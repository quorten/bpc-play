/* Miniature system call definitions for x86-64 Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_64.h"
#include "syscalls.h"

extern int errno;

void
_exit (int status)
{
  INTERNAL_SYSCALL_EXIT(__NR_exit, status);
}
