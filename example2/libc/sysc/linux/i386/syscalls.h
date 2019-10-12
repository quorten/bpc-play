/* See the COPYING file for license details.  */
#ifndef SYSCALLS_H
#define SYSCALLS_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

/* i386 Linux system call interface:

   Syscall instruction: int 80h
   Syscall number: eax
   Arguments 1-6: ebx ecx edx esi edi ebp
   Error: Embedded inside return value
   Return: eax

   Note that the special x86 privileged function calls can also be
   used for system calls, and purportedly that is faster, but heck,
   for simple assembly language programming, just stick to the trap
   (software interrupt) instruction.  */

#ifdef __GNUC__
#include "syscalls_gcc.h"
#endif

#ifndef INTERNAL_SYSCALL_0
#include "../syscalls_miss.h"
#endif

extern int errno;

#define INTERNAL_SYSCALL_ERROR_P(result, err) \
  ((long) result < 0)

#define CHECK_SET_ERRNO(err, result) \
  if (INTERNAL_SYSCALL_ERROR_P(result, err)) \
    { errno = -result; result = -1; } \
  else \
    errno = 0;

#define INLINE_SYSCALL_0(name, result) \
  long result; \
  INTERNAL_SYSCALL_0(name, err, result); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_1(name, result, arg1) \
  long result; \
  INTERNAL_SYSCALL_1(name, err, result, arg1); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_2(name, result, arg1, arg2) \
  long result; \
  INTERNAL_SYSCALL_2(name, err, result, arg1, arg2); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_3(name, result, arg1, arg2, arg3) \
  long result; \
  INTERNAL_SYSCALL_3(name, err, result, arg1, arg2, arg3); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_4(name, result, arg1, arg2, arg3, arg4) \
  long result; \
  INTERNAL_SYSCALL_4(name, err, result, arg1, arg2, arg3, arg4); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_5(name, result, arg1, arg2, arg3, arg4, arg5) \
  long result; \
  INTERNAL_SYSCALL_5(name, err, result, arg1, arg2, arg3, arg4, \
		     arg5); \
  CHECK_SET_ERRNO(err, result);

#define INLINE_SYSCALL_6(name, result, arg1, arg2, arg3, arg4, \
			 arg5, arg6) \
  long result; \
  INTERNAL_SYSCALL_6(name, err, result, arg1, arg2, arg3, arg4, \
		     arg5, arg6); \
  CHECK_SET_ERRNO(err, result);

#endif /* not SYSCALLS_H */
