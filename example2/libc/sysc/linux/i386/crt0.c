/* See the COPYING file for license details.  */

#include "../../../include/stdlib.h"

int main (int argc, char **argv, char **envp);

/* Note: The stack base is offset by one due to us assuming there is a
   return address on the stack when there actually is not.  */
void
_start (void *ofs_stack_base)
{
  char **stack = (char**)(&ofs_stack_base);
  int argc = *(long*)(stack - 1);
  char **argv = (char**)stack;
  char **envp;
  do
    stack++;
  while (*stack != 0);
  envp = (char**)(stack + 1);
  exit (main (argc, argv, envp));
}
