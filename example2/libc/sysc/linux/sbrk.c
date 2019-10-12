/* Miniature system call definitions for generic Linux.  */

/* See the COPYING file for license details.  */

#include "../../../include/errno.h"

/* Defined in brk.c.  */
extern void *__curbrk;
int brk (void *addr);

void *
sbrk (long increment)
{
  void *oldbrk;

  if (__curbrk == 0)
    if (brk (0) < 0) /* Initialize the break.  */
      return (void *) -1;

  if (increment == 0)
    return __curbrk;

  oldbrk = __curbrk;
  if (increment > 0) {
    if ((unsigned long) oldbrk +
	(unsigned long) increment < (unsigned long) oldbrk)
      return (void *) -1;
  } else {
    if ((unsigned long) oldbrk < (unsigned long) -increment)
      return (void *) -1;
  }
  if (brk (oldbrk + increment) == -1)
    return (void *) -1;

  return oldbrk;
}
