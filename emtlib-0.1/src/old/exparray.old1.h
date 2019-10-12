/* A simple expandable array implementation.
   How to use:

   * Whenever you want to use a specific type in an expandable array,
	 first call CREATE_EXP_ARRAY_TYPE(typename) with your type.

   * When you want to declare an expandable array, simply use
	 typename_array as the type name.

   * Initialize the array before use by calling INIT_ARRAY().  'array'
	 is the value of your array (use the syntax '*array' if it is a
	 pointer), and 'stride' is the allocation stride measured in
	 elements.  Whenever the array becomes full, the array will be
	 reallocated with that many more elements.

   * To add an element, first set the value at the position in the
	 data array specified by 'typename_array::num', then call
	 ADD_ARRAY_ELEMENT()

   * To shrink the allocated space in an array, call
	 NORMALIZE_ARRAY().  This function can also be used to reserve
	 space for several elements that will be stored in the array later
	 in time.

   * To insert an element at any position, first call
	 INSERT_ARRAY_ELEMENT(), then set the value at that position.

   * When you are done using an expandable array, make sure you
	 free(typename_array::d), or for extra and possibly redundant
	 saftey, call DESTROY_ARRAY().
*/

#ifndef EXPARRAY_H
#define EXPARRAY_H

#include <malloc.h>
#include <string.h>

#define CREATE_EXP_ARRAY_TYPE(typename)			\
	struct typename##_array_tag					\
	{											\
		typename* d;							\
		unsigned num;							\
	};											\
	typedef struct typename##_array_tag typename##_array;

#define INIT_ARRAY(typename, array, stride)						\
{																\
	(array).num = 0;											\
	(array).d = (typename*)malloc(sizeof(typename) * stride);	\
}

#define DESTROY_ARRAY(array)						\
{													\
	free((array).d);								\
	(array).d = NULL;								\
	(array).num = 0;								\
}

#define CHECK_ARRAY_REALLOC(typename, array, stride)					\
{																		\
	if ((array).num % stride == 0)										\
		(array).d = (typename*)realloc((array).d, sizeof(typename) *	\
									   ((array).num + stride));			\
}

#define ADD_ARRAY_ELEMENT(typename, array, stride)						\
{																		\
	(array).num++;														\
	if ((array).num % stride == 0)										\
		(array).d = (typename*)realloc((array).d, sizeof(typename) *	\
									   ((array).num + stride));			\
}

#define INSERT_ARRAY_ELEMENT(typename, array, stride, pos)				\
{																		\
	ADD_ARRAY_ELEMENT(typename, (array), stride);						\
	memmove(&(array).d[pos+1], &(array).d[pos], sizeof(typename) *		\
			((array).num - pos));										\
}

/* Delete an element by moving following elements over.  The array is
   not shrunken. */
#define DELETE_ARRAY_ELEMENT(typename, array, stride, pos)				\
{																		\
	memmove(&(array).d[pos], &(array).d[pos+1], sizeof(typename) *		\
			((array).num - (pos + 1)));									\
	(array).num--;														\
}

/* This function is used to ensure that the allocated space in the
   array is consistent with the standard algorithm. */
#define NORMALIZE_ARRAY(typename, array, stride)						\
	(array).d = (typename*)realloc((array).d, sizeof(typename) *		\
						   ((array).num + (stride - (array).num % stride)));

/* A handy macro that simplifies certain memory allocations (checkpoint) */
#define NUM_TO_CKPT(num, ckpt) (ckpt - num % ckpt)

#endif /* not EXPARRAY_H */
