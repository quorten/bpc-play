/* A simple expandable array implementation with length limit.

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
#define ELA_TYSIZE(array) sizeof(*((array).d))

/* However, under circumstances when sufficient type information is
   not available, the user may opt to include the `tysize' member in
   the structures by defining ELA_USE_TYSIZE.  */
#ifdef ELA_USE_TYSIZE
#define ELA_DEF_TYSIZE unsigned tysize;
#else
#define ELA_DEF_TYSIZE
#endif

#define ELA_TYPE(typename)						\
	struct typename##_array_tag					\
	{											\
		typename *d;							\
		unsigned len;							\
		ELA_DEF_TYSIZE							\
		/* User-defined fields.  */				\
		unsigned user1;							\
		unsigned maxlen;						\
	};											\
	typedef struct typename##_array_tag typename##_array

/* In order to avoid typecasting problems in C++, a generic array type
   must be defined for internal use by exparray.  This also simplifies
   some of our C code.  */
struct generic_array_tag
{
	unsigned char *d;
	unsigned len;
	ELA_DEF_TYSIZE
	/* User-defined fields.  */
	unsigned user1;
};
typedef struct generic_array_tag generic_array;

/* Aliases for user-defined fields.  */
#define ea_len_alloc user1

#ifdef ELA_USE_TYSIZE
#define ELA_INIT_TYSIZE (array).tysize = ELA_TYSIZE(array);
#else
#define ELA_INIT_TYSIZE
#endif

/* Initialize the given array.

   Parameters:

   `array'       the value (not pointer) of the array to modify
   `maxlen'      the maximum allowed array length, must be greater
                 than or equal to `reserve'
   `reserve'     the amount of memory to pre-allocate, typically 16.

   The array's "length" is always initialized to zero.  */
#define ELA_INIT(array, maxlen, reserve)						\
BRAY_STMT_START {												\
	/* assert(reserve >= 0); */									\
	generic_array *cpp_punk = (generic_array *)(&array);		\
	(array).len = 0;											\
	ELA_INIT_TYSIZE												\
	(array).ea_len_alloc = reserve;								\
	(array).maxlen = maxlen;									\
	cpp_punk->d = (unsigned char *)								\
		ea_malloc(ELA_TYSIZE(array) * (array).ea_len_alloc);	\
} BRAY_STMT_END

#ifdef ELA_USE_TYSIZE
#define ELA_DESTROY_TYSIZE (array).tysize = 0;
#else
#define ELA_DESTROY_TYSIZE
#endif

/* Destroy the given array.  This is mostly just a convenience
   function.  */
#define ELA_DESTROY(array)							\
BRAY_STMT_START {									\
	if ((array).d != NULL)							\
		ea_free((array).d);							\
	(array).d = NULL;								\
	(array).len = 0;								\
	ELA_DESTROY_TYSIZE								\
	(array).user1 = 0;								\
	(array).maxlen = 0;								\
} BRAY_STMT_END

/* Default exponential reallocators.  */
#define ELA_GROW(array)							\
BRAY_STMT_START {								\
	if ((array).len > (array).maxlen)			\
		abort();								\
	if ((array).len >= (array).ea_len_alloc)	\
	{											\
		generic_array *cpp_punk = (generic_array *)(&array);			\
		(array).ea_len_alloc <<= 1;										\
		cpp_punk->d = (unsigned char *)									\
			ea_realloc((array).d, ELA_TYSIZE(array) *					\
					   (array).ea_len_alloc);							\
	}																	\
} BRAY_STMT_END
#define ELA_NORMALIZE(array)										\
BRAY_STMT_START {													\
	generic_array *cpp_punk = (generic_array *)(&array);			\
	if ((array).len > (array).maxlen)								\
		abort();													\
	while ((array).ea_len_alloc > 0 &&								\
		   (array).ea_len_alloc >= (array).len)						\
		(array).ea_len_alloc >>= 1;									\
	if ((array).ea_len_alloc == 0)									\
		(array).ea_len_alloc = 1;									\
	while ((array).len >= (array).ea_len_alloc)						\
		(array).ea_len_alloc <<= 1;									\
	cpp_punk->d = (unsigned char *)									\
		ea_realloc((array).d, ELA_TYSIZE(array) *					\
				   (array).ea_len_alloc);							\
} BRAY_STMT_END

/* "Safe referencing" macro to get an element at the given index.  */
#define ELA_SR(array, x) (array).d[x]

/* "Safe bounds referencing" macro to get an element at the given
   index, with bounds checking.  */
#define ELA_SBR(array, x) \
	(((x) >= 0 && (x) < (array).len) ? ELA_SR(array, x) : abort())

/*********************************************************************
   The few necessary primitive functions were specified above.  Next
   follows the convenience functions.  */

#define ELA_ADD(array) \
	BRAY_ADD(ELA, array)
#define ELA_INS(array, pos) \
	BRAY_INS(ELA, array, pos)
#define ELA_APPEND(array, element) \
	BRAY_APPEND(ELA, array, element)
#define ELA_INSERT(array, pos, element) \
	BRAY_INSERT(ELA, array, pos, element)
#define ELA_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(ELA, array, data, num_data)
#define ELA_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(ELA, array, pos, data, num_data)
#define ELA_PREPEND(array, element) \
	BRAY_PREPEND(ELA, array, element)
#define ELA_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(ELA, array, data, num_data)
#define ELA_REMOVE(array, pos) \
	BRAY_REMOVE(ELA, array, pos)
#define ELA_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(ELA, array, pos)
#define ELA_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(ELA, array, pos, num_data)
#define ELA_SET_SIZE(array, size) \
	BRAY_SET_SIZE(ELA, array, size)
#define ELA_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(ELA, array, element)
#define ELA_POP_BACK(array) \
	BRAY_POP_BACK(ELA, array)
#define ELA_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(ELA, array, element)
#define ELA_POP_FRONT(array) \
	BRAY_POP_FRONT(ELA, array)
#define ELA_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(ELA, array, begin, end)
#define ELA_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(ELA, array, pos)
#define ELA_SWAP(left, right) \
	BRAY_SWAP(ELA, left, right)
#define ELA_FRONT(array) \
	BRAY_FRONT(ELA, array)
#define ELA_BACK(array) \
	BRAY_BACK(ELA, array)
#define ELA_BEGIN(array) \
	BRAY_BEGIN(ELA, array)
#define ELA_END(array) \
	BRAY_END(ELA, array)
#define ELA_CLELAR(array) \
	BRAY_CLELAR(ELA, array)
#define ELA_EMPTY(array) \
	BRAY_EMPTY(ELA, array)

/********************************************************************/

/* Linear reallocators.  */
#define ea_stride user1 /* User-defined field alias */
#define ELA1_GROW(array)												\
BRAY_STMT_START {														\
	generic_array *cpp_punk = (generic_array *)(&array);				\
	if ((array).len > (array).maxlen)									\
		abort();														\
	if ((array).len % (array).ea_stride == 0)							\
		cpp_punk->d = (unsigned char *)									\
			ea_realloc((array).d, ELA_TYSIZE(array) *					\
					   ((array).len + (array).ea_stride));				\
} BRAY_STMT_END
#define ELA1_NORMALIZE(array)											\
BRAY_STMT_START {														\
	generic_array *cpp_punk = (generic_array *)(&array);				\
	if ((array).len > (array).maxlen)									\
		abort();														\
	cpp_punk->d = (unsigned char *)										\
		ea_realloc((array).d, ELA_TYSIZE(array) *						\
	   ((array).len + ((array).ea_stride - (array).len % (array).ea_stride))); \
} BRAY_STMT_END

#define ELA1_TYSIZE(array) ELA_TYSIZE(array)
#define ELA1_TYPE(typename) ELA_TYPE(typename)
#define ELA1_INIT(array, reserve) ELA_INIT(array, reserve)
#define ELA1_DESTROY(array) ELA_DESTROY(array)

#define ELA1_SR(array, x) (array).d[x]

#define ELA1_ADD(array) \
	BRAY_ADD(ELA1, array)
#define ELA1_INS(array, pos) \
	BRAY_INS(ELA1, array, pos)
#define ELA1_APPEND(array, element) \
	BRAY_APPEND(ELA1, array, element)
#define ELA1_INSERT(array, pos, element) \
	BRAY_INSERT(ELA1, array, pos, element)
#define ELA1_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(ELA1, array, data, num_data)
#define ELA1_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(ELA1, array, pos, data, num_data)
#define ELA1_PREPEND(array, element) \
	BRAY_PREPEND(ELA1, array, element)
#define ELA1_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(ELA1, array, data, num_data)
#define ELA1_REMOVE(array, pos) \
	BRAY_REMOVE(ELA1, array, pos)
#define ELA1_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(ELA1, array, pos)
#define ELA1_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(ELA1, array, pos, num_data)
#define ELA1_SET_SIZE(array, size) \
	BRAY_SET_SIZE(ELA1, array, size)
#define ELA1_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(ELA1, array, element)
#define ELA1_POP_BACK(array) \
	BRAY_POP_BACK(ELA1, array)
#define ELA1_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(ELA1, array, element)
#define ELA1_POP_FRONT(array) \
	BRAY_POP_FRONT(ELA1, array)
#define ELA1_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(ELA1, array, begin, end)
#define ELA1_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(ELA1, array, pos)
#define ELA1_SWAP(left, right) \
	BRAY_SWAP(ELA1, left, right)
#define ELA1_FRONT(array) \
	BRAY_FRONT(ELA1, array)
#define ELA1_BACK(array) \
	BRAY_BACK(ELA1, array)
#define ELA1_BEGIN(array) \
	BRAY_BEGIN(ELA1, array)
#define ELA1_END(array) \
	BRAY_END(ELA1, array)
#define ELA1_CLELA1R(array) \
	BRAY_CLELA1R(ELA1, array)
#define ELA1_EMPTY(array) \
	BRAY_EMPTY(ELA1, array)

#endif /* not EXPARRAY_H */
