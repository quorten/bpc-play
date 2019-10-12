/* GCC compiler syntax for inline system call macros.  */

/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GCC_H
#define SYSCALLS_GCC_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

/* We do use one trick here, because the ARM OABI is a bit tricky.
   Let's explain it out with a good example.

   INTERNAL_SYSCALL_EXIT(__NR_exit, status)
   ->
     register long __r0 __asm__("r0") = status;
     asm volatile ("swi	%1	@ syscall " "__NR_exit"
		   : : "i" (__NR_exit), "r" (__r0)
		   : "memory");

   Here, we are using a quirk in the C preprocessor stringification in
   order to get the desired effect.  At "@ syscall" line, we want the
   name of the syscall symbol to be embedded, but when we pass as an
   argument to the GCC assembler constraints, we want the value to be
   substituted.

*/

#define INTERNAL_SYSCALL_0(name, err, __r0) \
  register long __r0 __asm__("r0"); \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name) \
		: "memory");

/* Takes one argument, never returns, so there is no result.  */
/* N.B. We must write this assembler so that it takes a return
   address, otherwise we'll get a complaint along the lines "cannot
   represent this relocation in this object code format."  */
#define INTERNAL_SYSCALL_EXIT(name, arg1) \
  register long __r0 __asm__("r0") = (long)arg1; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0) \
		: "memory");

#define INTERNAL_SYSCALL_1(name, err, __r0, arg1) \
  register long __r0 __asm__("r0") = (long)arg1; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0) \
		: "memory");

#define INTERNAL_SYSCALL_2(name, err, __r0, arg1, arg2) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0), "r" (__r1) \
		: "memory");

#define INTERNAL_SYSCALL_3(name, err, __r0, arg1, arg2, arg3) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0), "r" (__r1), "r" (__r2) \
		: "memory");

#define INTERNAL_SYSCALL_4(name, err, __r0, arg1, arg2, arg3, \
			   arg4) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r3") = (long)arg4; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3) \
		: "memory");

#define INTERNAL_SYSCALL_5(name, err, __r0, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r3") = (long)arg4; \
  register long __r4 __asm__("r4") = (long)arg5; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3), "r" (__r4) \
		: "memory");

#define INTERNAL_SYSCALL_6(name, err, __r0, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __r0 __asm__("r0") = (long)arg1; \
  register long __r1 __asm__("r1") = (long)arg2; \
  register long __r2 __asm__("r2") = (long)arg3; \
  register long __r3 __asm__("r3") = (long)arg4; \
  register long __r4 __asm__("r4") = (long)arg5; \
  register long __r5 __asm__("r5") = (long)arg6; \
  asm volatile ("swi	%1	@ syscall " #name \
		: "=r" (__r0) \
		: "i" (name), "r" (__r0), "r" (__r1), "r" (__r2), \
		  "r" (__r3), "r" (__r4), "r" (__r5) \
		: "memory");

#endif /* not SYSCALLS_GCC_H */
