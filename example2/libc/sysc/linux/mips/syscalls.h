/* See the COPYING file for license details.  */
#ifndef SYSCALLS_H
#define SYSCALLS_H

/* MIPS n32 || n64 Linux system call interface:

   Syscall instruction: syscall
   Syscall number: v0
   Arguments 1-6: a0 a1 a2 a3 a4 a5
   Error: a3
   Return: v0

   MIPS o32 Linux system call interface:

   Syscall instruction: syscall
   Syscall number: v0
   Arguments 1-4: a0 a1 a2 a3
   Arguments 5-7: sp+16 sp+20 sp+24
   Error: a3
   Return: v0

   Note that `sp == $29'.  It is recommended to (1) allocate
   (i.e. subtract) 32 bytes on the stack before (2) storing the
   arguments to the stack for the system call, as that is what glibc,
   uClibc, Musl libc, etc. all do.  Also, please note that glibc and
   uClibc do some magic to force the frame pointer to be used. in the
   interest of making debugging correct when around the system call,
   but (at least early versions of) Musl libc does not, so this is not
   strictly necessary.

           subu sp, 32
           sw arg5, 16(sp)
           sw arg6, 20(sp)
           sw arg7, 24(sp)
           syscall

*/

/* N.B.  Are you unsure if you are targeting o32 or n32/n64?  No
   worries, it is possible to write up the system calls so that they
   are ABI-agnostic, at the expense of being slower.  That is what we
   do by default.  */

#ifdef __GNUC__
#include "syscalls_gcc.h"
#endif

#endif /* not SYSCALLS_H */
