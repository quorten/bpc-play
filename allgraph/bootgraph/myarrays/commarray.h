/* commarray.h -- Common array manipulation functions in generic
   template format.

Public Domain 2010, 2011, 2012, 2013, 2014, 2015, 2017,
2018, 2019 Andrew Makousky

*/

/*

Specify the allocator abbreviation to use when calling these routines
as the first parameter, and token-pasting will be used accordingly to
reference the specific implementations.

These specifics must be defined for a particular allocation method.
Substitute ALBR for the corresponding allocator abbreviation.

* ALBR_TYSIZE
* ALBR_TYPE
* ALBR_INIT
* ALBR_DESTROY
* ALBR_GROW
* ALBR_NORMALIZE
* ALBR_SR

*/

#ifndef COMM_BRAY_H
#define COMM_BRAY_H

#include <string.h>

/* Courtesy of GLib et al.  (The idea originated from lots of earlier
   software, but I don't remember them all.)

   Provide simple macro statement wrappers:
     BRAY_STMT_START { statements; } BRAY_STMT_END;
   This can be used as a single statement, like:
     if (x) BRAY_STMT_START { ... } BRAY_STMT_END; else ...
   This intentionally does not use compiler extensions like GCC's
   '({...})' to avoid portability issues or side effects when compiled
   with different compilers.
*/
#define BRAY_STMT_START do
#define BRAY_STMT_END while (0)

/* Increment the size of the array and allocate space for one new item
   in the array.  The expected calling convention of this form is to
   first set the value of the new element in the array by doing
   something like `array.d[array.num] = value;', then calling
   `BRAY_ADD()'.  Expandable arrays always reserve space for one item
   beyond the current size of the array.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify */
#define BRAY_ADD(ALBR, array)					\
BRAY_STMT_START {								\
	(array).len++;								\
	ALBR##_GROW(array);							\
} BRAY_STMT_END

/* Increment the size of the array, allocate space for one new item,
   and move elements in the array to make space for inserting an item
   at the given position.  The expected calling convention of this
   form is similar to that of `BRAY_ADD()', only the value of the item
   should be set after this call rather than before it.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `pos'         the zero-based index of where the new item will be
                 inserted */
#define BRAY_INS(ALBR, array, pos)					\
BRAY_STMT_START {									\
	/* assert(pos >= 0 && pos <= (array).len); */						\
	unsigned len_end = (array).len - pos;								\
	BRAY_ADD(ALBR, array);												\
	memmove(&ALBR##_SR(array, pos+1), &ALBR##_SR(array, pos),			\
			ALBR##_TYSIZE(array) * len_end);							\
} BRAY_STMT_END

/* Appends the given item to the end of the array.  Note that the
   appended item cannot be larger than an integer type.  To append a
   larger item, use `BRAY_APPEND_MULT()' with the quantity set to one.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `element'     the value of the item to append */
#define BRAY_APPEND(ALBR, array, element)								\
BRAY_STMT_START {														\
	ALBR##_SR(array, (array).len) = element;							\
	BRAY_ADD(ALBR, array);												\
} BRAY_STMT_END

/* Inserts the given item at the indicated position.  Elements after
   this position will get moved back.  Note that the inserted item
   cannot be larger than an integer type.  To insert a larger item,
   use `BRAY_INSERT_MULT()' with the quantity set to one.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `pos'         the zero-based index indicating where to insert the
                 given element
   `element'     the value of the item to append */
#define BRAY_INSERT(ALBR, array, pos, element)							\
BRAY_STMT_START {														\
	unsigned sos = pos; /* avoid macro evaluation problems */			\
	BRAY_INS(ALBR, array, sos);											\
	ALBR##_SR(array, sos) = element;									\
} BRAY_STMT_END

/* Append multiple items at one time.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `data'        the address of the items to append
   `num_len'     the number of items to append */
#define BRAY_APPEND_MULT(ALBR, array, data, num_data)					\
BRAY_STMT_START {														\
	/* assert(data != NULL); */											\
	/* assert(num_data >= 0); */										\
	unsigned pos = (array).len;											\
	(array).len += num_data;											\
	ALBR##_NORMALIZE(array);											\
	memcpy(&ALBR##_SR(array, pos), data, ALBR##_TYSIZE(array) * num_data); \
} BRAY_STMT_END

/* Insert multiple items at one time.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `pos'         the zero-based index indicating where to insert the
                 given element
   `data'        the address of the items to insert
   `num_data'    the number of items to insert */
#define BRAY_INSERT_MULT(ALBR, array, pos, data, num_data)				\
BRAY_STMT_START {														\
	/* assert(pos >= 0 && pos <= (array).len); */						\
	/* assert(data != NULL); */											\
	/* assert(num_data >= 0); */										\
	unsigned sos = pos; /* avoid macro evaluation problems */			\
	unsigned len_end = (array).len - sos;								\
	(array).len += num_data;											\
	ALBR##_NORMALIZE(array);											\
	memmove(&ALBR##_SR(array, sos+num_data), &ALBR##_SR(array, sos),	\
			ALBR##_TYSIZE(array) * len_end);							\
	memcpy(&ALBR##_SR(array, sos), data, ALBR##_TYSIZE(array) * num_data); \
} BRAY_STMT_END

/* Prepend the given element at the beginning of the array.  Note that
   the prepended item cannot be larger than an integer type.  To
   prepend a larger item, use `BRAY_PREPEND_MULT()' with the quantity
   set to one.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `element'     the value of the element to append */
#define BRAY_PREPEND(ALBR, array, element) \
	BRAY_INSERT(ALBR, array, 0, element)

/* Prepend multiple elements at one time.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `data'        the address of the elements to prepend
   `num_data'    the number of items to prepend */
#define BRAY_PREPEND_MULT(ALBR, array, data, num_data) \
	BRAY_INSERT_MULT(ALBR, array, 0, data, num_data)

/* Delete the indexed element by moving following elements over the
   indexed element.  The allocated array memory is not shrunken.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `pos'         the zero-based index indicating which element to
                 remove */
#define BRAY_REMOVE(ALBR, array, pos)									\
BRAY_STMT_START {														\
	/* assert((array).len > 0); */										\
	/* assert(pos >= 0 && pos < (array).len); */						\
	unsigned end = pos + 1;												\
	unsigned len_end = (array).len - end;								\
	memmove(&ALBR##_SR(array, pos), &ALBR##_SR(array, end),				\
			ALBR##_TYSIZE(array) * len_end);							\
	(array).len--;														\
} BRAY_STMT_END

/* Delete the indexed element by moving the last element into the
   indexed position.  The allocated array memory is not shrunken.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `pos'         the zero-based index indicating which element to
                 remove */
#define BRAY_REMOVE_FAST(ALBR, array, pos)								\
BRAY_STMT_START {														\
	/* assert((array).len > 0); */										\
	memcpy(&ALBR##_SR(array, pos), &ALBR##_SR(array, (array).len-1),	\
		   ALBR##_TYSIZE(array));										\
	(array).len--;														\
} BRAY_STMT_END

/* Remove multiple consecutive of elements by moving the following
   elements over the deleted elements.  The allocated array memory is
   not shrunken.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `data'        the address of the data array to append
   `pos'         the zero-based index indicating the first element to
                 remove
   `num_data'    the number of items to remove */
#define BRAY_REMOVE_MULT(ALBR, array, pos, num_data)					\
BRAY_STMT_START {														\
	/* assert(pos >= 0 && pos <= (array).len); */						\
	/* assert(num_data >= 0); */										\
	unsigned end = pos + num_data;										\
	/* assert(end <= (array).len); */									\
	unsigned len_end = (array).len - end;								\
	memmove(&ALBR##_SR(array, pos), &ALBR##_SR(array, end),				\
			ALBR##_TYSIZE(array) * len_end);							\
	(array).len = pos + len_end;										\
} BRAY_STMT_END

/* Set an array's size and allocate enough memory for that size.

   Parameters:

   `ALBR'        the allocator methods to use
   `array'       the value (not pointer) of the exparray to modify
   `size'        the new size of the array, measured in elements */
#define BRAY_SET_SIZE(ALBR, array, size)			\
BRAY_STMT_START {									\
	/* assert(size >= 0); */						\
	(array).len = size;								\
	ALBR##_NORMALIZE(ALBR, array);					\
} BRAY_STMT_END

/********************************************************************/
/* The following routines are defined for compatibility with C++
   standard template library containers.  */

/* Push an element onto the end of the given array, as if it were a
   stack.  This function is just an alias for `BRAY_APPEND()'.  */
#define BRAY_PUSH_BACK(ALBR, array, element) \
	BRAY_APPEND(ALBR, array, element)

/* Pop an element off of the end of the given array, as if it were a
   stack.  The allocated array memory is not shrunken.  C++ allows for
   the size to go negative here, but our implementation does not.  */
#define BRAY_POP_BACK(ALBR, array) \
	/* assert((array).len > 0); */ \
	(array).len--

/* This function is just an alias for `BRAY_PREPEND()'.  You should not
   use this macro for a stack data structure, as it will have serious
   performance problems when used in that way.  */
#define BRAY_PUSH_FRONT(ALBR, array, element) \
	BRAY_PREPEND(ALBR, array, element)

/* Remove an element from the beginning of the given array.  The
   allocated array memory is not shrunken.  You should not use this
   macro for a stack data structure, as it will have serious
   performance problems when used in that way.  C++ allows for the
   size to go negative here, but our implementation does not.  */
#define BRAY_POP_FRONT(ALBR, array) \
	/* assert((array).len > 0); */ \
	BRAY_REMOVE(ALBR, array, 0)

/* Remove multiple consecutive of elements by moving the following
   elements over the deleted elements.  The allocated array memory is
   not shrunken.

   Parameters:

   `array'       the value (not pointer) of the exparray to modify
   `begin'       the zero-based index indicating the first element to
                 remove
   `end'         the zero-based index indicating the next element to
                 keep, or the size of the array if no following
                 elements are to be kept */
#define BRAY_ERASE_RANGE(ALBR, array, begin, end) \
BRAY_STMT_START {														\
	/* assert(begin >= 0 && begin <= (array).len); */					\
	/* assert(end >= 0 && end <= (array).len); */						\
	/* assert(begin <= end); */											\
	unsigned len_end = (array).len - end;								\
	memmove(&ALBR##_SR(array, begin), &ALBR##_SR(array, end),			\
			ALBR##_TYSIZE(array) * len_end);							\
	(array).len = begin + len_end;										\
} BRAY_STMT_END

/* Remove all elements starting at a zero-based index and going until
   the end of the array.  This is just a convenience function for
   `BRAY_ERASE_RANGE()'.  */
#define BRAY_ERASE_AFTER(ALBR, array, pos) \
	BRAY_ERASE_RANGE(ALBR, array, pos, (array).len)

/* "Rename" exparrays to be arrays based off of the opposite array's
   data elements.  */
#define BRAY_SWAP(ALBR, left, right)				\
BRAY_STMT_START {									\
	generic_array temp;								\
	memcpy(temp, left, sizeof(generic_array));		\
	memcpy(left, right, sizeof(generic_array));		\
	memcpy(right, temp, sizeof(generic_array));		\
} BRAY_STMT_END

/* Get the element at the beginning of the array.  */
#define BRAY_FRONT(ALBR, array) ALBR##_SR(array, 0)

/* Get the element at the end of the array.  */
#define BRAY_BACK(ALBR, array) \
	ALBR##_SR(array, ((array).len == 0 ? 0 : (array).len - 1))

/* Get an index to the first element of the array.  */
#define BRAY_BEGIN(ALBR, array) 0

/* Get an index just beyond the last element of the array.  */
#define BRAY_END(ALBR, array) ((array).len)

/* Clear the contents of an array.  The allocated space is not
   shrunken.  */
#define BRAY_CLEAR(ALBR, array) ((array).len = 0)

/* Test if an array is empty.  This is just an alias for testing
   whether the length is zero.  */
#define BRAY_EMPTY(ALBR, array) ((array).len == 0)

#endif /* not COMM_BRAY_H */
