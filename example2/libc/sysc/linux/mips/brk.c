/* Miniature system call definitions for x86-64 Linux.  */

/* See the COPYING file for license details.  */

#include "unistd_32.h"
#include "syscalls.h"
#include "../../../include/errno.h"

void *__curbrk = 0;

int
brk (void *addr)
{
  void *result;
  INTERNAL_SYSCALL_1(__NR_brk, __a3, __v0, addr);
  result = (void*)__v0;

  __curbrk = result;

  if (result < addr) {
    errno = ENOMEM;
    return -1;
  }

  return 0;
}
