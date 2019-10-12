/* GCC compiler syntax for inline system call macros.  */

/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GCC_H
#define SYSCALLS_GCC_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

#define INTERNAL_SYSCALL_0(name, err, result) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: : "memory", "cc");

/* Takes one argument, never returns, so there is no result.  */
#define INTERNAL_SYSCALL_EXIT(name, arg1) \
  asm volatile ("int $0x80" \
		: : "a" (name), "b" (arg1));

#define INTERNAL_SYSCALL_1(name, err, result, arg1) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: "a" (name), "b" (arg1) \
		: "memory", "cc");

#define INTERNAL_SYSCALL_2(name, err, result, arg1, arg2) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: "a" (name), "b" (arg1), "c" (arg2) \
		: "memory", "cc");

#define INTERNAL_SYSCALL_3(name, err, result, arg1, arg2, arg3) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: "a" (name), "b" (arg1), "c" (arg2), "d" (arg3) \
		: "memory", "cc");

#define INTERNAL_SYSCALL_4(name, err, result, arg1, arg2, arg3, arg4) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: "a" (name), "b" (arg1), "c" (arg2), "d" (arg3), \
		  "S" (arg4) \
		: "memory", "cc");

#define INTERNAL_SYSCALL_5(name, err, result, arg1, arg2, arg3, \
			   arg4, arg5) \
  asm volatile ("int $0x80" \
		: "=a" (result) \
		: "a" (name), "b" (arg1), "c" (arg2), "d" (arg3), \
		  "S" (arg4), "D" (arg5) \
		: "memory", "cc");

#define INTERNAL_SYSCALL_6(name, err, result, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  asm volatile ("push %%ebp\n\t" \
		"movl %7, %%ebp\n\t" \
		"int $0x80\n\t" \
		"pop %%ebp" \
		: "=a" (result) \
		: "a" (name), "b" (arg1), "c" (arg2), "d" (arg3), \
		  "S" (arg4), "D" (arg5), "m" (arg6) \
		: "memory", "cc");

#endif /* not SYSCALLS_GCC_H */
