/* Miniature system call definitions for MIPS32 Linux.  */

/* See the COPYING file for license details.  */

#include "../../../include/stdlib.h"
#include "../../../include/time.h"
#include "unistd_32.h"
#include "syscalls.h"

time_t
time (time_t *out)
{
  INTERNAL_SYSCALL_1(__NR_time, __a3, __v0, 0);
  if (out != NULL)
    *out = (time_t)__v0;
  return (time_t)__v0;
}
