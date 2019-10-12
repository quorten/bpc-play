/* Simple random number generator.  Unfortunately, using a simple
   algorithm also causes results for which simple and easy-to-detect
   patterns can be identified.  That should be kind of obvious, of
   course.  */

/* See the COPYING file for license details.  */
/* Special thanks to the DJGPP libc library source code for providing
   the code of this simple random number generator.  The copied
   portions are trivial enough (and unoriginal enough) for copyright
   to not be applicable.  Hence, this code is technically in public
   domain.  */

#include "../include/stdlib.h"

static unsigned long long next = 1;

int
rand (void)
{
  /* This multiplier was obtained from Knuth, D.E., "The Art of
     Computer Programming," Vol 2, Seminumerical Algorithms, Third
     Edition, Addison-Wesley, 1998, p. 106 (line 26) & p. 108.  */
  next = next * 6364136223846793005LL + 1;
  return (int)((next >> 21) & RAND_MAX);
}

void
srand (unsigned int seed)
{
  next = seed;
}
