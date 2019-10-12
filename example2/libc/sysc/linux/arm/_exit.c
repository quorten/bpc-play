/* Miniature system call definitions for ARM EABI Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"

extern int errno;

void
_exit (int status)
{
  INTERNAL_SYSCALL_EXIT(__NR_exit, status);
}
