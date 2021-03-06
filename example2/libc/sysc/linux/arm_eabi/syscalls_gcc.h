/* GCC compiler syntax for inline system call macros.  */

/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GCC_H
#define SYSCALLS_GCC_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

#define INTERNAL_SYSCALL_0(name, err, __r0) \
  register long __r0 __asm__("r0"); \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7) \
		: "memory");

/* Takes one argument, never returns, so there is no result.  */
#define INTERNAL_SYSCALL_EXIT(name, arg1) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: : "r" (__r7), "r" (__r0) \
		: "memory");

#define INTERNAL_SYSCALL_1(name, err, __r0, arg1) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0) \
		: "memory");

#define INTERNAL_SYSCALL_2(name, err, __r0, arg1, arg2) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0), "r" (__r1) \
		: "memory");

#define INTERNAL_SYSCALL_3(name, err, __r0, arg1, arg2, arg3) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0), "r" (__r1), "r" (__r2) \
		: "memory");

#define INTERNAL_SYSCALL_4(name, err, __r0, arg1, arg2, arg3, \
			   arg4) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r2") = (long)arg4; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3) \
		: "memory");

#define INTERNAL_SYSCALL_5(name, err, __r0, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r2") = (long)arg4; \
  register long __r4 __asm__("r2") = (long)arg5; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3), "r" (__r4) \
		: "memory");

#define INTERNAL_SYSCALL_6(name, err, __r0, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r2") = (long)arg4; \
  register long __r4 __asm__("r2") = (long)arg5; \
  register long __r5 __asm__("r2") = (long)arg6; \
  register long __r7 __asm__("r7") = name; \
  asm volatile ("swi	#0" \
		: "=r" (__r0) \
		: "r" (__r7), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3), "r" (__r4), "r" (__r5) \
		: "memory");

#endif /* not SYSCALLS_GCC_H */
