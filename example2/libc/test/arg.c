/* See the COPYING file for license details.  */

#include <stdio.h>
#include <stdlib.h>

void putnum (int num);

int
main (int argc, char *argv[], char **envp)
{
  putnum (argc);
  putchar ('\n');
  while (*argv != NULL) {
    puts (*argv++);
  }
  while (*envp != NULL) {
    puts (*envp++);
  }
  return 0;
}
