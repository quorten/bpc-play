/* See the COPYING file for license details.  */

#include "../include/stdio.h"
#include "../include/unistd.h"

void
abort (void)
{
  char msg[] = "Aborted\n";
  register unsigned int i;
  for (i = 0; i < 8; i++)
    putchar (msg[i]);
  _exit (134);
}
