/* A simple fixed-allocation, variable-length array implementation.

Public Domain 2019 Andrew Makousky

*/

/* Note that since we share the same initialize-in-place allocation
   functions as `exparray', we define the actual memory allocation to
   be `maxlen + 1'.  This makes sense for common case usage, i.e. a
   256-byte buffer for a string supports up to 255 ASCII characters.
   If you intend the buffer allocation to be a power-of-two, then
   subtract one from your requested `maxlen'.  */

#ifndef FIXARRAY_H
#define FIXARRAY_H

/* For `abort()' */
#include <stdlib.h>

/* This neat trick can be used to determine the element size of an
   expandable array type without requiring the user to explicitly
   specify the type name or size.  Of course, the array must have a
   pointer type indicative of the element size.  */
#define FA_TYSIZE(array) sizeof(*((array).d))

#define FA_MAXLEN(array) \
	(sizeof((array).d) / sizeof((array).d[0] - 1)

#define FA_TYPE(typename, maxlen)				\
	struct typename##maxlen##_array_tag			\
	{											\
		typename d[maxlen+1];					\
		unsigned len;							\
	};											\
	typedef struct typename##maxlen##_array_tag typename##maxlen##_array

/* Initialize the given array.

   Parameters:

   `array'       the value (not pointer) of the array to modify

   The array's "length" is always initialized to zero.  */
#define FA_INIT(array) \
	(array).len = 0;

/* Destroy the given array.  This is mostly just a convenience
   function.  */
#define FA_DESTROY(array) \
	(array).len = 0;

#define FA_GROW(array)							\
BRAY_STMT_START {								\
	if ((array).len > FA_MAXLEN(array))			\
		abort();								\
} BRAY_STMT_END
#define FA_NORMALIZE(array)						\
BRAY_STMT_START {								\
	if ((array).len > FA_MAXLEN(array))			\
		abort();								\
} BRAY_STMT_END

/* "Safe referencing" macro to get an element at the given index.  */
#define FA_SR(array, x) (array).d[x]

/* "Safe bounds referencing" macro to get an element at the given
   index, with bounds checking.  */
#define FA_SBR(array, x) \
	(((x) >= 0 && (x) < (array).len) ? FA_SR(array, x) : abort())

/*********************************************************************
   The few necessary primitive functions were specified above.  Next
   follows the convenience functions.  */

#define FA_ADD(array) \
	BRAY_ADD(FA, array)
#define FA_INS(array, pos) \
	BRAY_INS(FA, array, pos)
#define FA_APPEND(array, element) \
	BRAY_APPEND(FA, array, element)
#define FA_INSERT(array, pos, element) \
	BRAY_INSERT(FA, array, pos, element)
#define FA_APPEND_MULT(array, data, num_data) \
	BRAY_APPEND_MULT(FA, array, data, num_data)
#define FA_INSERT_MULT(array, pos, data, num_data) \
	BRAY_INSERT_MULT(FA, array, pos, data, num_data)
#define FA_PREPEND(array, element) \
	BRAY_PREPEND(FA, array, element)
#define FA_PREPEND_MULT(array, data, num_data) \
	BRAY_PREPEND_MULT(FA, array, data, num_data)
#define FA_REMOVE(array, pos) \
	BRAY_REMOVE(FA, array, pos)
#define FA_REMOVE_FAST(array, pos) \
	BRAY_REMOVE_FAST(FA, array, pos)
#define FA_REMOVE_MULT(array, pos, num_data) \
	BRAY_REMOVE_MULT(FA, array, pos, num_data)
#define FA_SET_SIZE(array, size) \
	BRAY_SET_SIZE(FA, array, size)
#define FA_PUSH_BACK(array, element) \
	BRAY_PUSH_BACK(FA, array, element)
#define FA_POP_BACK(array) \
	BRAY_POP_BACK(FA, array)
#define FA_PUSH_FRONT(array, element) \
	BRAY_PUSH_FRONT(FA, array, element)
#define FA_POP_FRONT(array) \
	BRAY_POP_FRONT(FA, array)
#define FA_ERASE_RANGE(array, begin, end) \
	BRAY_ERASE_RANGE(FA, array, begin, end)
#define FA_ERASE_AFTER(array, pos) \
	BRAY_ERASE_AFTER(FA, array, pos)
#define FA_SWAP(left, right) \
	BRAY_SWAP(FA, left, right)
#define FA_FRONT(array) \
	BRAY_FRONT(FA, array)
#define FA_BACK(array) \
	BRAY_BACK(FA, array)
#define FA_BEGIN(array) \
	BRAY_BEGIN(FA, array)
#define FA_END(array) \
	BRAY_END(FA, array)
#define FA_CLFAR(array) \
	BRAY_CLFAR(FA, array)
#define FA_EMPTY(array) \
	BRAY_EMPTY(FA, array)

#endif /* not FIXARRAY_H */
