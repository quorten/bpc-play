/* See the COPYING file for license details.  */

#include "../../../include/stdlib.h"

int main (int argc, char **argv, char **envp);

/* Since ARM passes arguments in registers, we have to copy the stack
   pointer to the first register parameter to use our convenient C
   argument unpacking code.  */
asm (".text");
asm (".globl	_start");
asm (".type	_start, %function");
asm ("_start:");
/* asm ("mov	r0, sp"); */
asm ("mov	r0, r13");
/* We can just "fall through" rather than needing to call/jump.  */
/* asm ("j	_start2"); */
asm (".size	_start, .-_start");

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
