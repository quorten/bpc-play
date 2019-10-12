/* A simple expandable array implementation.

Public Domain 2010, 2011, 2012, 2013, 2014, 2015, 2017, 2018,
2019 Andrew Makousky

*/

#ifndef EXPARRAY_H
#define EXPARRAY_H

/* For `abort()' */
#include <stdlib.h>

#include "commarray.h"

#ifndef ea_malloc
#include "xmalloc.h"
#define ea_malloc  xmalloc
#define ea_realloc xrealloc
#define ea_free    free /* Don't need extra safety */
#endif

/* This neat trick can be used to determine the element size of an
   expandable array type without requiring the user to explicitly
   specify the type name or size.  Of course, the array must have a
   pointer type indicative of the element size.  */
#define EA_TYSIZE(array) sizeof(*((array).d))

/* However, under circumstances when sufficient type information is
   not available, the user may opt to include the `tysize' member in
   the structures by defining EA_USE_TYSIZE.  */
#ifdef EA_USE_TYSIZE
#define EA_DEF_TYSIZE unsigned tysize;
#else
#define EA_DEF_TYSIZE
#endif

#define EA_TYPE(typename)						\
	struct typename##_array_tag					\
	{											\
		typename *d;							\
		unsigned len;							\
		EA_DEF_TYSIZE							\
		/* User-defined fields.  */				\
		unsigned user1;							\
	};											\
	typedef struct typename##_array_tag typename##_array

/* In order to avoid typecasting problems in C++, a generic array type
   must be defined for internal use by exparray.  This also simplifies
   some of our C code.  */
struct generic_array_tag
{
	unsigned char *d;
	unsigned len;
	EA_DEF_TYSIZE
	/* User-defined fields.  */
	unsigned user1;
};
typedef struct generic_array_tag generic_array;

/* Aliases for user-defined fields.  */
#define ea_len_alloc user1

#ifdef EA_USE_TYSIZE
#define EA_INIT_TYSIZE (array).tysize = EA_TYSIZE(array);
#else
#define EA_INIT_TYSIZE
#endif

/* Initialize the given array.

   Parameters:

   `array'       the value (not pointer) of the array to modify
   `reserve'     the amount of memory to pre-allocate, typically 16.

   The array's "length" is always initialized to zero.  */
#define EA_INIT(array, reserve)									\
BRAY_STMT_START {												\
	/* assert(reserve >= 0); */									\
	generic_array *cpp_punk = (generic_array *)(&array);		\
	(array).len = 0;											\
	EA_INIT_TYSIZE												\
	(array).ea_len_alloc = reserve;								\
	cpp_punk->d = (unsigned char *)								\
		ea_malloc(EA_TYSIZE(array) * (array).ea_len_alloc);		\
} BRAY_STMT_END

#ifdef EA_USE_TYSIZE
#define EA_DESTROY_TYSIZE (array).tysize = 0;
#else
#define EA_DESTROY_TYSIZE
#endif

/* Destroy the given array.  This is mostly just a convenience
   function.  */
#define EA_DESTROY(array)							\
BRAY_STMT_START {									\
	if ((array).d != NULL)							\
		ea_free((array).d);							\
	(array).d = NULL;								\
	(array).len = 0;								\
	EA_DESTROY_TYSIZE								\
	(array).user1 = 0;								\
} BRAY_STMT_END

/* Default exponential reallocators.  */
#define EA_GROW(array)							\
BRAY_STMT_START {								\
	if ((array).len >= (array).ea_len_alloc)	\
	{											\
		generic_array *cpp_punk = (generic_array *)(&array);			\
		(array).ea_len_alloc <<= 1;										\
		cpp_punk->d = (unsigned char *)									\
			ea_realloc((array).d, EA_TYSIZE(array) *					\
					   (array).ea_len_alloc);							\
	}																	\
} BRAY_STMT_END
#define EA_NORMALIZE(array)											\
BRAY_STMT_START {													\
	generic_array *cpp_punk = (generic_array *)(&array);			\
	while ((array).ea_len_alloc > 0 &&								\
		   (array).ea_len_alloc >= (array).len)						\
		(array).ea_len_alloc >>= 1;									\
	if ((array).ea_len_alloc == 0)									\
		(array).ea_len_alloc = 1;									\
	while ((array).len >= (array).ea_len_alloc)						\
		(array).ea_len_alloc <<= 1;									\
	cpp_punk->d = (unsigned char *)									\
		ea_realloc((array).d, EA_TYSIZE(array) *					\
				   (array).ea_len_alloc);							\
} BRAY_STMT_END

/* "Safe referencing" macro to get an element at the given index.  */
#define EA_SR(array, x) (array).d[x]

/* "Safe bounds referencing" macro to get an element at the given
   index, with bounds checking.  */
#define EA_SBR(array, x) \
	(((x) >= 0 && (x) < (array).len) ? EA_SR(array, x) : abort())

/*********************************************************************
   The few necessary primitive functions were specified above.  Next
   follows the convenience functions.  */

#define EA_ADD(array) \
	BRAY_ADD(EA, array)
#define EA_INS(array, pos) \
	BRAY_INS(EA, array, pos)
#define EA_APPEND(array, element) \
	BRAY_APPEND(EA, array, element)
#define EA_INSERT(array, pos, element) \
	BRAY_INSERT(EA, array, pos, element)
#define EA_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(EA, array, data, num_data)
#define EA_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(EA, array, pos, data, num_data)
#define EA_PREPEND(array, element) \
	BRAY_PREPEND(EA, array, element)
#define EA_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(EA, array, data, num_data)
#define EA_REMOVE(array, pos) \
	BRAY_REMOVE(EA, array, pos)
#define EA_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(EA, array, pos)
#define EA_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(EA, array, pos, num_data)
#define EA_SET_SIZE(array, size) \
	BRAY_SET_SIZE(EA, array, size)
#define EA_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(EA, array, element)
#define EA_POP_BACK(array) \
	BRAY_POP_BACK(EA, array)
#define EA_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(EA, array, element)
#define EA_POP_FRONT(array) \
	BRAY_POP_FRONT(EA, array)
#define EA_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(EA, array, begin, end)
#define EA_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(EA, array, pos)
#define EA_SWAP(left, right) \
	BRAY_SWAP(EA, left, right)
#define EA_FRONT(array) \
	BRAY_FRONT(EA, array)
#define EA_BACK(array) \
	BRAY_BACK(EA, array)
#define EA_BEGIN(array) \
	BRAY_BEGIN(EA, array)
#define EA_END(array) \
	BRAY_END(EA, array)
#define EA_CLEAR(array) \
	BRAY_CLEAR(EA, array)
#define EA_EMPTY(array) \
	BRAY_EMPTY(EA, array)

/********************************************************************/

/* Linear reallocators.  */
#define ea_stride user1 /* User-defined field alias */
#define EA1_GROW(array)													\
BRAY_STMT_START {														\
	generic_array *cpp_punk = (generic_array *)(&array);				\
	if ((array).len % (array).ea_stride == 0)							\
		cpp_punk->d = (unsigned char *)									\
			ea_realloc((array).d, EA_TYSIZE(array) *					\
					   ((array).len + (array).ea_stride));				\
} BRAY_STMT_END
#define EA1_NORMALIZE(array)											\
BRAY_STMT_START {														\
	generic_array *cpp_punk = (generic_array *)(&array);				\
	cpp_punk->d = (unsigned char *)										\
		ea_realloc((array).d, EA_TYSIZE(array) *						\
	   ((array).len + ((array).ea_stride - (array).len % (array).ea_stride))); \
} BRAY_STMT_END

#define EA1_TYSIZE(array) EA_TYSIZE(array)
#define EA1_TYPE(typename) EA_TYPE(typename)
#define EA1_INIT(array, reserve) EA_INIT(array, reserve)
#define EA1_DESTROY(array) EA_DESTROY(array)

#define EA1_SR(array, x) (array).d[x]

#define EA1_ADD(array) \
	BRAY_ADD(EA1, array)
#define EA1_INS(array, pos) \
	BRAY_INS(EA1, array, pos)
#define EA1_APPEND(array, element) \
	BRAY_APPEND(EA1, array, element)
#define EA1_INSERT(array, pos, element) \
	BRAY_INSERT(EA1, array, pos, element)
#define EA1_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(EA1, array, data, num_data)
#define EA1_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(EA1, array, pos, data, num_data)
#define EA1_PREPEND(array, element) \
	BRAY_PREPEND(EA1, array, element)
#define EA1_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(EA1, array, data, num_data)
#define EA1_REMOVE(array, pos) \
	BRAY_REMOVE(EA1, array, pos)
#define EA1_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(EA1, array, pos)
#define EA1_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(EA1, array, pos, num_data)
#define EA1_SET_SIZE(array, size) \
	BRAY_SET_SIZE(EA1, array, size)
#define EA1_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(EA1, array, element)
#define EA1_POP_BACK(array) \
	BRAY_POP_BACK(EA1, array)
#define EA1_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(EA1, array, element)
#define EA1_POP_FRONT(array) \
	BRAY_POP_FRONT(EA1, array)
#define EA1_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(EA1, array, begin, end)
#define EA1_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(EA1, array, pos)
#define EA1_SWAP(left, right) \
	BRAY_SWAP(EA1, left, right)
#define EA1_FRONT(array) \
	BRAY_FRONT(EA1, array)
#define EA1_BACK(array) \
	BRAY_BACK(EA1, array)
#define EA1_BEGIN(array) \
	BRAY_BEGIN(EA1, array)
#define EA1_END(array) \
	BRAY_END(EA1, array)
#define EA1_CLEA1R(array) \
	BRAY_CLEA1R(EA1, array)
#define EA1_EMPTY(array) \
	BRAY_EMPTY(EA1, array)

/********************************************************************/

/* Note: GLib GArray defs appear not to have much practical use, so
   leave the code here but do not define it at compile-time for
   now.  */
#if 0 /* GLib GArray defs */

#include <string.h>

/* GLib GArray reallocators.  */
#define EGA_GROW(array)													\
BRAY_STMT_START {														\
	struct _GRealWrap *reala = (struct _GRealWrap *)(&array);			\
	if (reala->len + (reala->zero_term ? 1 : 0) >= reala->ea_len_alloc) \
	{																	\
		reala->ea_len_alloc <<= 1;										\
		reala->data = (guchar *)ea_realloc(reala->data, reala->tysize *	\
										  reala->ea_len_alloc);			\
		if (reala->clear)												\
		{																\
			memset(reala->data + reala->tysize * reala->len, 0,			\
				   reala->tysize * (reala->ea_len_alloc - reala->len)); \
		}																\
		else if (reala->zero_term)										\
		{																\
			memset(reala->data + reala->tysize * reala->len, 0,			\
				   reala->tysize * 1);									\
		}																\
	}																	\
} BRAY_STMT_END
#define EGA_NORMALIZE(array)										\
BRAY_STMT_START {													\
	struct _GRealWrap *reala = (struct _GRealWrap *)(&array);		\
	while (reala->ea_len_alloc >= reala->len + (reala->zero_term ? 1 : 0)) \
		reala->ea_len_alloc >>= 1;									\
	if ((reala)->ea_len_alloc == 0)									\
		(reala)->ea_len_alloc = 1;									\
	while (reala->len + (reala->zero_term ? 1 : 0) >= reala->ea_len_alloc) \
		reala->ea_len_alloc <<= 1;									\
	reala->data = (guchar *)ea_realloc(reala->data, reala->tysize *	\
									  reala->ea_len_alloc);			\
	if (reala->clear)												\
	{																\
		memset(reala->data + reala->tysize * reala->len, 0,			\
			   reala->tysize * (reala->ea_len_alloc - reala->len)); \
	}																\
	else if (reala->zero_term)										\
	{																\
		memset(reala->data + reala->tysize * reala->len, 0,			\
			   reala->tysize * 1);									\
	}																\
} BRAY_STMT_END

#define EGA_TYSIZE(array) EA_TYSIZE(array)
#define EGA_TYPE(typename) EA_TYPE(typename)
#define EGA_INIT(array, reserve) EA_INIT(array, reserve)
#define EGA_DESTROY(array) EA_DESTROY(array)

#define EGA_SR(array, x) (*((array).d + (array).tysize * (x)))

#define EGA_ADD(array) \
	BRAY_ADD(EGA, array)
#define EGA_INS(array, pos) \
	BRAY_INS(EGA, array, pos)
#define EGA_APPEND(array, element) \
	BRAY_APPEND(EGA, array, element)
#define EGA_INSERT(array, pos, element) \
	BRAY_INSERT(EGA, array, pos, element)
#define EGA_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(EGA, array, data, num_data)
#define EGA_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(EGA, array, pos, data, num_data)
#define EGA_PREPEND(array, element) \
	BRAY_PREPEND(EGA, array, element)
#define EGA_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(EGA, array, data, num_data)
#define EGA_REMOVE(array, pos) \
	BRAY_REMOVE(EGA, array, pos)
#define EGA_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(EGA, array, pos)
#define EGA_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(EGA, array, pos, num_data)
#define EGA_SET_SIZE(array, size) \
	BRAY_SET_SIZE(EGA, array, size)
#define EGA_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(EGA, array, element)
#define EGA_POP_BACK(array) \
	BRAY_POP_BACK(EGA, array)
#define EGA_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(EGA, array, element)
#define EGA_POP_FRONT(array) \
	BRAY_POP_FRONT(EGA, array)
#define EGA_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(EGA, array, begin, end)
#define EGA_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(EGA, array, pos)
#define EGA_SWAP(left, right) \
	BRAY_SWAP(EGA, left, right)
#define EGA_FRONT(array) \
	BRAY_FRONT(EGA, array)
#define EGA_BACK(array) \
	BRAY_BACK(EGA, array)
#define EGA_BEGIN(array) \
	BRAY_BEGIN(EGA, array)
#define EGA_END(array) \
	BRAY_END(EGA, array)
#define EGA_CLEGAR(array) \
	BRAY_CLEGAR(EGA, array)
#define EGA_EMPTY(array) \
	BRAY_EMPTY(EGA, array)
#endif /* GLib GArray defs */

#endif /* not EXPARRAY_H */
