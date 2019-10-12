/* Miniature system call definitions for x86 32-bit Linux.  Note that
   this will also work on x86-64 Linux systems that support the x32
   ABI.  */

/* See the COPYING file for license details.  */

#include "../../../include/stdlib.h"
#include "../../../include/time.h"
#include "unistd_32.h"
#include "syscalls.h"

time_t
time (time_t *out)
{
  time_t result;
  INTERNAL_SYSCALL_1(__NR_time, , result, 0);
  if (out != NULL)
    *out = result;
  return result;
}
