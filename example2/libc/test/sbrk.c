/* See the COPYING file for license details.  */

#include <stdio.h>
#include <unistd.h>

void putnum (int num);

int
main (void)
{
  char *src = "Hello there.";
  void *start = sbrk (0);
  void *next;
  char *dest = start;
  unsigned i;
  putnum ((long) start); putchar ('\n');
  /* Now this can be really weird.  On FreeBSD, the first call to
     `sbrk ()' appears to allocate a whole big chunk of memory, so the
     `next' pointer will not be equal to the `start' pointer there
     like it is on Linux.  */
  next = sbrk (4096);
  putnum ((long) next); putchar ('\n');
  if (next == (void *) -1)
    return 1;
  next = sbrk (0);
  putnum ((long) next); putchar ('\n');
  while (*src)
    *dest++ = *src++;
  puts ((char *) start);
  return 0;
}
