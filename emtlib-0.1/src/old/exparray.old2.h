/* A simple expandable array implementation.

   How to use:

   * This implementation was intentionally designed to be close to
     GLib's GArray implementation.  Therefore, you should be able to
     almost automatically convert code from GArray to exparray.

   * Whenever you want to use a specific type in an expandable array,
	 first call ea_type(typename) with your type.

   * When you want to declare an expandable array, simply use
	 typename_array as the type name.

   * Initialize the array before use by calling ea_init().  'array' is
	 the value of your array (use the syntax '*array' if it is a
	 pointer), and 'stride' is the allocation stride measured in
	 elements.  Whenever the array becomes full, the array will be
	 reallocated with that many more elements.

   * To append an element, call ea_append().

   * To shrink the allocated space in an array, call ea_normalize().
	 This function can also be used to reserve space for several
	 elements that will be stored in the array later in time.

   * To insert an element at any position, call ea_insert().  Items at
     or after this position will get moved back.

   * When you are done using an expandable array, make sure you call
	 free(typename_array::d), or for extra and possibly redundant
	 saftey, call ea_destroy().

   * You can also modify allocation behavior by overriding the macros
     ea_grow and ea_normalize.

   * Some functions in here aren't documented.  If they aren't, then
     they might have been added simply to help with compatibility with
     GArray.  See the GLib documentation for GArray to find out how to
     use them.
*/

#ifndef EXPARRAY_H
#define EXPARRAY_H

#include <malloc.h>
#include <string.h>

#define TYPECAST void

#define ea_type(typename)						\
	struct typename##_array_tag					\
	{											\
		typename* d;							\
		unsigned num;							\
		unsigned tysize;						\
		unsigned stride;						\
	};											\
	typedef struct typename##_array_tag typename##_array;

/* Define a wrapper type for a GLib GArray.  */
#define ea_wtype(typename)						\
	struct typename##_array_tag					\
	{											\
		typename* d;							\
		unsigned num;							\
	};											\
	typedef struct typename##_array_tag typename##_array;

#define ea_init(typename, _stride, array)						\
{																\
	(array).num = 0;											\
	(array).tysize = sizeof(typename);							\
	(array).stride = _stride;									\
	(array).d = (TYPECAST*)malloc((array).tysize *				\
								  (array).stride);				\
}

#define ea_destroy(array)			\
{													\
	free((array).d);								\
	(array).d = NULL;								\
	(array).num = 0;								\
}

/* Reallocation function for single step growth.  */
#define ea_grow(array)								\
{																		\
	if ((array).num % (array).stride == 0)								\
		(array).d = (TYPECAST*)realloc((array).d, (array).tysize *		\
									   ((array).num + (array).stride));	\
}

#define ea_append(array, element)						\
{																		\
	(array).d[(array).num] = element;									\
	(array).num++;														\
	ea_grow(array);									\
}

/* In this macro, "data" should be a value, not an address.  "len" is
   the number of elements in "data".  */
#define ea_append_mult(array, data, len)				\
{																		\
	unsigned pos = (array).num;											\
	(array).num += len;													\
	ea_normalize(array);								\
	memcpy(&(array).d[pos], &(data), (array).tysize * len);				\
}

#define ea_insert(array, pos, element)				\
{																		\
	(array).num++;														\
	ea_grow(array);									\
	memmove(&(array).d[pos+1], &(array).d[pos], (array).tysize *		\
			((array).num - pos));										\
	(array).d[pos] = element;											\
}

#define ea_insert_mult(array, pos, data, len)			\
{																		\
	(array).num += len;													\
	ea_normalize(array);								\
	memmove(&(array).d[pos+len], &(array).d[pos], (array).tysize *		\
			((array).num - pos));										\
	memcpy(&(array).d[pos], &(data), (array).tysize * len);				\
}

#define ea_prepend(array, element)	\
	ea_insert(array, 0, element);

#define ea_prepend_mult(array, data, len)	\
	ea_insert_mult(array, 0, data, len);

/* Delete an element by moving following elements over.  The array is
   not shrunken.  */
#define ea_remove(array, pos)							\
{																		\
	memmove(&(array).d[pos], &(array).d[pos+1], (array).tysize *		\
			((array).num - (pos + 1)));									\
	(array).num--;														\
}

/* Delete an element by moving the last element into the given index.
   The array is not shrunken.  */
#define ea_remove_fast(array, pos)					\
{																		\
	memmove(&(array).d[pos], &(array).d[(array).num-1],					\
			(array).tysize);											\
	(array).num--;														\
}

#define ea_remove_range(array, pos, len)				\
{																		\
	memmove(&(array).d[pos], &(array).d[pos+len], (array).tysize *		\
			((array).num - (pos + len)));								\
}

/* This function is used to ensure that the allocated space in the
   array is consistent with the standard algorithm.  */
#define ea_normalize(array)							\
	(array).d = (TYPECAST*)realloc((array).d, (array).tysize *			\
		   ((array).num + ((array).stride - (array).num % (array).stride)));

#define ea_set_size(array, size)	\
{													\
	(array).num = size;								\
	ea_normalize(array);			\
}

/* A handy macro that simplifies certain memory allocations (checkpoint). */
/* This might just be boilerplate code.  */
#define NUM_TO_CKPT(num, ckpt) (ckpt - num % ckpt)

#endif /* not EXPARRAY_H */
