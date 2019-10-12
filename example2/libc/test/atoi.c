/* See the COPYING file for license details.  */

#include <stdio.h>
#include <stdlib.h>

void putnum (int num);

int
main (int argc, char *argv[], char **envp)
{
  if (argc != 2)
    return 1;
  putnum (atoi (argv[1]));
  putchar ('\n');
  return 0;
}
