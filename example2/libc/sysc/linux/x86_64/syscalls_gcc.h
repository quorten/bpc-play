/* GCC compiler syntax for inline system call macros.  */

/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GCC_H
#define SYSCALLS_GCC_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

/* This is clearly some sort of bug of shortcoming in `gcc' that I'd
   have to report to get fixed later.  If it is really true that the
   Linux x86_64 ABI is specified to use the full 64-bit width of the
   registers, then leaving the high-order bits of the 64-bit registers
   uninitialized when loading new data into them could cause random
   bugs under some circumstances.

   But hey, it appears to still work correctly, so why not leave it
   as-is?

   It appears that `glibc' and other more established `libc'
   implementations just do direct assembly language rather than
   compiler wrapping as they do for i386.  That was probably their
   short-term reaction to work around compiler bugs, as was often the
   case with historic compilers.  Some things never change...

   UPDATE: Good old i386 GCC assembler constraints that are easy to
   use have not been fully ported to x86_64.  Rather, you must use the
   "modern-way" that is used for ARM and MIPS where you define a
   register variable by name.  `long' on x86_64 is 64-bits wide, so
   this is a nice and well assurance for us.

*/

#define INTERNAL_SYSCALL_0(name, err, result) \
  asm volatile ("syscall" \
		: "=a" (result) \
		: : "memory", "cc", "r11", "cx");

/* Takes one argument, never returns, so there is no result.  */
#define INTERNAL_SYSCALL_EXIT(name, arg1) \
  asm volatile ("syscall" \
		: : "a" (name), "D" (arg1));

#define INTERNAL_SYSCALL_1(name, err, result, arg1) \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "D" (arg1) \
		: "memory", "cc", "r11", "cx");

#define INTERNAL_SYSCALL_2(name, err, result, arg1, arg2) \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "D" (arg1), "S" (arg2) \
		: "memory", "cc", "r11", "cx");

#define INTERNAL_SYSCALL_3(name, err, result, arg1, arg2, arg3) \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "D" (arg1), "S" (arg2), "d" (arg3) \
		: "memory", "cc", "r11", "cx");

#define INTERNAL_SYSCALL_4(name, err, result, arg1, arg2, arg3, \
			   arg4) \
  register long __rdi __asm__("rdi") = (long)arg1; \
  register long __rsi __asm__("rsi") = (long)arg2; \
  register long __rdx __asm__("rdx") = (long)arg3; \
  register long __r10 __asm__("r10") = (long)arg4; \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "r" (arg1), "r" (arg2), "r" (arg3), \
		  "r" (arg4) \
		: "memory", "cc", "r11", "cx");

#define INTERNAL_SYSCALL_5(name, err, result, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __rdi __asm__("rdi") = (long)arg1; \
  register long __rsi __asm__("rsi") = (long)arg2; \
  register long __rdx __asm__("rdx") = (long)arg3; \
  register long __r10 __asm__("r10") = (long)arg4; \
  register long __r8 __asm__("r8") = (long)arg5; \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "r" (arg1), "r" (arg2), "r" (arg3), \
		  "r" (arg4), "r" (arg5) \
		: "memory", "cc", "r11", "cx");

#define INTERNAL_SYSCALL_6(name, err, result, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __rdi __asm__("rdi") = (long)arg1; \
  register long __rsi __asm__("rsi") = (long)arg2; \
  register long __rdx __asm__("rdx") = (long)arg3; \
  register long __r10 __asm__("r10") = (long)arg4; \
  register long __r8 __asm__("r8") = (long)arg5; \
  register long __r9 __asm__("r9") = (long)arg6; \
  asm volatile ("syscall" \
		: "=a" (result) \
		: "a" (name), "r" (arg1), "r" (arg2), "r" (arg3), \
		  "r" (arg4), "r" (arg5), "r" (arg6) \
		: "memory", "cc", "r11", "cx");

#endif /* not SYSCALLS_GCC_H */
