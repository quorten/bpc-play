/* See the COPYING file for license details.  */
#ifndef SYSCALLS_GEN_H
#define SYSCALLS_GEN_H

/* Note that the design of this syscalls.h header is inspired from GNU
   libc, uClibc, and Musl libc, but we try to avoid what I would
   consider is excessive trickery that distracts from the purpose of
   this library to also be educational and friendly toward hands-on
   tinkering.  */

#ifdef GEN_IMPL
/* GEN_IMPL */

#define _syscall0(type, name, nr_name) \
type name(void) \
{ \
  INLINE_SYSCALL_0(nr_name, result); \
  return (type) result; \
}

#define _syscall1(type, name, nr_name, type1, arg1) \
type name(type1 arg1) \
{ \
  INLINE_SYSCALL_1(nr_name, result, arg1); \
  return (type) result; \
}

#define _syscall2(type, name, nr_name, \
		  type1, arg1, type2, arg2) \
type name(type1 arg1, type2 arg2) \
{ \
  INLINE_SYSCALL_2(nr_name, result, arg1, arg2); \
  return (type) result; \
}

#define _syscall3(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3) \
type name(type1 arg1, type2 arg2, type3 arg3) \
{ \
  INLINE_SYSCALL_3(nr_name, result, arg1, arg2, arg3); \
  return (type) result; \
}

#define _syscall4(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
  INLINE_SYSCALL_4(nr_name, result, arg1, arg2, arg3, arg4); \
  return (type) result; \
}

#define _syscall5(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4, type5, arg5) \
type name(type1 arg1, type2 arg2, type3 arg3, \
	  type4 arg4, type5 arg5) \
{ \
  INLINE_SYSCALL_5(nr_name, result, arg1, arg2, arg3, arg4, arg5); \
  return (type) result; \
}

#define _syscall6(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4, type5, arg5, type6, arg6) \
type name(type1 arg1, type2 arg2, type3 arg3, \
	  type4 arg4, type5 arg5, type6 arg6) \
{ \
  INLINE_SYSCALL_6(nr_name, result, arg1, arg2, arg3, arg4, arg5, arg6); \
  return (type) result; \
}

#else
/* GEN_DECL */

#define _syscall0(type, name, nr_name) \
type name(void)

#define _syscall1(type, name, nr_name, type1, arg1) \
type name(type1 arg1)

#define _syscall2(type, name, nr_name, \
		  type1, arg1, type2, arg2) \
type name(type1 arg1, type2 arg2)

#define _syscall3(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3) \
type name(type1 arg1, type2 arg2, type3 arg3)

#define _syscall4(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4)

#define _syscall5(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4, type5, arg5) \
type name(type1 arg1, type2 arg2, type3 arg3, \
	  type4 arg4, type5 arg5)

#define _syscall6(type, name, nr_name, \
		  type1, arg1, type2, arg2, type3, arg3, \
		  type4, arg4, type5, arg5, type6, arg6) \
type name(type1 arg1, type2 arg2, type3 arg3, \
	  type4 arg4, type5 arg5, type6 arg6)

#endif /* GEN_IMPL */

#endif /* not SYSCALLS_GEN_H */
