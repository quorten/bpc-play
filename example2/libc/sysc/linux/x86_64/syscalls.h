/* See the COPYING file for license details.  */
#ifndef SYSCALLS_H
#define SYSCALLS_H

#ifdef __GNUC__
#include "syscalls_gcc.h"
#endif

/* x86_64 Linux system call interface:

   Syscall instruction: syscall
   Syscall number: rax
   Arguments 1-6: rdi rsi rdx r10 r8 r9
   Error: Embedded inside return value
   Return: rax

*/

#endif /* not SYSCALLS_H */
