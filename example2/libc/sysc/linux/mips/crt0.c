/* See the COPYING file for license details.  */

#include "../../../include/stdlib.h"

int main (int argc, char **argv, char **envp);

/* Since MIPS passes arguments in registers, we have to copy the stack
   pointer to the first register parameter to use our convenient C
   argument unpacking code.  */
asm (".text");
asm (".globl	__start");
asm (".type	__start, @function");
asm ("__start:");
/* asm ("move	$a0, $sp"); */
asm ("move	$4, $29");
/* We can just "fall through" rather than needing to call/jump.  */
/* asm ("j	_start2"); */
asm (".size	__start, .-__start");

void
_start2 (char **stack)
{
  int argc = *(long*)stack;
  char **argv;
  char **envp;
  stack++;
  argv = (char**)stack;
  do
    stack++;
  while (*stack != 0);
  envp = (char**)(stack + 1);
  exit (main (argc, argv, envp));
}
