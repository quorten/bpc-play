/* "Fully fixed array" -- An array with both a fixed length and
   allocation.

Public Domain 2019 Andrew Makousky

*/

#ifndef FULFIXAR_H
#define FULFIXAR_H

/* Fully fixed array.  Both the allocation and the length of the array
   are fixed and identical.  This is mainly some helper macros to make
   generics programming easier.  */
#define FFA_TYPE(typename, length) \
	struct typename##_ar##length##_tag			\
	{											\
		typename d[length];						\
	};											\
	typedef struct typename##_ar##length##_tag typename##_ar##length

#define FFA_LEN(array) \
	(sizeof((array).d) / sizeof((array).d[0]))

#endif /* not FULFIXAR_H */
