/* GCC compiler syntax for inline system call macros.  */

/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GCC_H
#define SYSCALLS_GCC_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

#if !defined(_MIPS_SIM)
/* This is a hack, but it works.  Technically "$8" and "$9" do get
   clobbered, but we avoid saying so that we can compile our system
   call inline assembler.  */
#define __SYSCALL_CLOBBERS "$1", "$3", "$10", "$11", "$12", "$13", \
    "$14", "$15", "$24", "$25", "memory"
#elif _MIPS_SIM == _ABIO32
/* o32 */
#define __SYSCALL_CLOBBERS "$1", "$3", "$8", "$9", "$10", "$11", \
    "$12", "$13", "$14", "$15", "$24", "$25", "memory"
#else
/* n32 || n64 */
#define __SYSCALL_CLOBBERS "$1", "$3", "$10", "$11", "$12", "$13", \
    "$14", "$15", "$24", "$25", "memory"
#endif /* _MIPS_SIM */

#define INTERNAL_SYSCALL_0(name, __a3, __v0) \
  register long __v0 __asm__("$2"); \
  register long __a3 __asm__("$7"); \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "=r" (__a3) \
		: "I" (name) \
		: __SYSCALL_CLOBBERS);

/* Takes one argument, never returns, so there is no result.  */
/* N.B. We must write this assembler so that it takes a return
   address, otherwise we'll get a complaint along the lines "operand
   number out of range."  */
#define INTERNAL_SYSCALL_EXIT(name, arg1) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a3 __asm__("$7"); \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "=r" (__a3) \
		: "I" (name), "r" (__a0));

#define INTERNAL_SYSCALL_1(name, __a3, __v0, arg1) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a3 __asm__("$7"); \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "=r" (__a3) \
		: "I" (name), "r" (__a0) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_2(name, __a3, __v0, arg1, arg2) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a3 __asm__("$7"); \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "=r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_3(name, __a3, __v0, arg1, arg2, arg3) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7"); \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "=r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_4(name, __a3, __v0, arg1, arg2, arg3, arg4) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2) \
		: __SYSCALL_CLOBBERS);

#if !defined(_MIPS_SIM)
/* ABI-agnostic hack.  */

#define INTERNAL_SYSCALL_5(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  register long __a4 __asm__("$8") = (long)arg5; \
  asm volatile (".set noreorder\n\t" \
		"subu	$29, 32\n\t" /* $29 == $sp */ \
		"sw	%6, 16($29)\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		"addiu	$29, 32\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" (__a4) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_6(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  register long __a4 __asm__("$8") = (long)arg5; \
  register long __a5 __asm__("$9") = (long)arg6; \
  asm volatile (".set noreorder\n\t" \
		"subu	$29, 32\n\t" /* $29 == $sp */ \
		"sw	%6, 16($29)\n\t" \
		"sw	%7, 20($29)\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		"addiu	$29, 32\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" (__a4), "r" (__a5) \
		: __SYSCALL_CLOBBERS);

#elif _MIPS_SIM == _ABIO32
/* o32 */

#define INTERNAL_SYSCALL_5(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  asm volatile (".set noreorder\n\t" \
		"subu	$29, 32\n\t" /* $29 == $sp */ \
		"sw	%6, 16($29)\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		"addiu	$29, 32\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" ((long)arg5) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_6(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  asm volatile (".set noreorder\n\t" \
		"subu	$29, 32\n\t" /* $29 == $sp */ \
		"sw	%6, 16($29)\n\t" \
		"sw	%7, 20($29)\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		"addiu	$29, 32\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" ((long)arg5), "r" ((long)arg6) \
		: __SYSCALL_CLOBBERS);

#else
/* n32 || n64 */

#define INTERNAL_SYSCALL_5(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  register long __a4 __asm__("$8") = (long)arg5; \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" (__a4) \
		: __SYSCALL_CLOBBERS);

#define INTERNAL_SYSCALL_6(name, __a3, __v0, arg1, arg2, arg3, \
			   arg4, arg5, arg6) \
  register long __v0 __asm__("$2"); \
  register long __a0 __asm__("$4") = (long)arg1; \
  register long __a1 __asm__("$5") = (long)arg2; \
  register long __a2 __asm__("$6") = (long)arg3; \
  register long __a3 __asm__("$7") = (long)arg4; \
  register long __a4 __asm__("$8") = (long)arg5; \
  register long __a5 __asm__("$9") = (long)arg6; \
  asm volatile (".set noreorder\n\t" \
		"li	$2, %2\n\t" /* $2 == $v0 */ \
		"syscall\n\t" \
		".set reorder" \
		: "=r" (__v0), "+r" (__a3) \
		: "I" (name), "r" (__a0), "r" (__a1), "r" (__a2), \
		  "r" (__a4), "r" (__a5) \
		: __SYSCALL_CLOBBERS);

#endif /* _MIPS_SIM */

#endif /* not SYSCALLS_GCC_H */
