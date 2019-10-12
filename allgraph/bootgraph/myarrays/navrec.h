/* Helper macros for navigating forward and backward in memory through
   structures.

Public Domain 2018, 2019 Andrew Makousky

See the file "UNLICENSE" in the top level directory for details.

*/

/* You could argue that the primary purpose of these utility macros is
   to avoid using too much C-style pointer arithmetic within the main
   drag of your code.  Indeed that is one of the primary purposes.
   The secondary purpose is to make it easy to search for locations in
   source code where pointer arithmetic is being performed: just
   search for the `NAR_' namespace prefix.  */

#ifndef NAVREC_H
#define NAVREC_H

/* This neat trick can be used to determine the element size of a
   pointer type without requiring the user to explicitly specify the
   type name or size.  */
#define NAR_TYSIZE(ptr) sizeof(*(ptr))

/* Navigate backwards by indicated size in bytes, cast the resulting
   pointer to `typename *'.  */
#define NAR_PREV(typename, ptr, size) \
	((typename *)((char *)ptr - size))

/* Navigate forwards by indicated size in bytes, cast the resulting
   pointer to `typename *'.  */
#define NAR_NEXT(typename, ptr, size) \
	((typename *)((char *)ptr + size))

/* Navigate backwards by `sizeof(typename)', cast the resulting
   pointer to `typename'.  Useful for fetching a pointer to a
   structure just before the current structure.  */
#define NAR_PREV_REC(typename, ptr) \
	NAR_PREV(typename, ptr, sizeof(typename))

/* Navigate forwards by `sizeof(*ptr)', cast the resulting pointer to
   `typename'.  Useful for fetching a pointer to a structure just
   after the current structure.  */
#define NAR_NEXT_REC(typename, ptr) \
	NAR_NEXT(typename, ptr, NAR_TYSIZE(ptr))

/* Assuming `ptr' structure is opaque, compute the address of the
   first field of type `typename'.  (This is silly if the structure is
   known at compile-time.)  */
#define NAR_FIRST_FIELD(typename, ptr) \
	((typename *)ptr)

/* Assuming `ptr' structure is opaque and does not contain padding at
   the end of its size, compute the address of the last field of type
   `typename'.  (This is silly if the structure is known at
   compile-time.)  */
#define NAR_LAST_FIELD(typename, ptr) \
	((typename *)((char *)ptr + NAR_TYSIZE(ptr) - sizeof(typename)))

/* Compute the address of the stated structure member.  (This is silly
   if the structure is known at compile-time.)  */
#define NAR_NAMED_FIELD(typename, ptr, field) \
	((typename *)&(ptr->field))

/* In case there is no definition of offsetof() provided - though any
   proper Standard C system should have one.  Inspired by PCRE library
   and GLib.

   Note: Normally we'd use `size_t', but this may not be defined, so
   we use `unsigned long'.  */
#if !(defined(__GNUC__) && __GNUC__ >= 4) && !defined(offsetof)
#define offsetof(typename, field) \
	((unsigned long)NAR_NAMED_FIELD(char, (typename *)0, field))
#endif

/* Given a structure type, a pointer to a structure field, and the
   name of the field, compute the address of the beginning of the
   structure.  */
#define NAR_REC_FROM_FIELD(typename, ptr, field) \
	NAR_PREV(typename, ptr, offsetof(typename, field))

/* NAR_MEM_ALIGN is the largest machine integer size of the current
   platform, which is generally the size that memory should be aligned
   to.

   NOTE: GLib defines G_MEM_ALIGN so that it is computed ahead of
   compile-time, during the platform-specific configure step.  The
   pre-computed value is then stored as a preprocessor macro.  Here,
   for the sake of source code simplicity, we assume that the compiler
   can evaluate this to a constant at compile-time.  The main
   compromise here is that that legacy compilers may not be up to that
   par, which is probably the reason why GLib was programmed the way
   it was.  */
#define NAR_MEM_ALIGN \
	((sizeof(void *) > sizeof(long)) ? sizeof(void *) : sizeof(long))

/* Remainder of `size' that needs to be filled to be divisible by
   `align'.  Key assumption: `align' must be a power of two.  */
#define NAR_REM_ALIGN(size, align) \
	(size & (align - 1))

/* Round an arbitrary size up to the given memory alignment size.  */
/* TODO FIXME: Inefficient as a macro, we should use a function so we
   can store `NAR_REM_ALIGN()'.  */
#define NAR_ALIGN_SIZE(size, align) \
	(NAR_REM_ALIGN(size, align) ? \
	 (size + (align - NAR_REM_ALIGN(size, align))) : size)

/* Zero the most significant bit of the given unsigned integer type
   width and variable.  The type must be an unsigned integer type or
   else this macro will not function properly.  */
#define NAR_NO_HIBIT(typename, val) ((val) & ((typename)-1 >> 1))

/********************************************************************/

/* Compatibility macro for GLib, return a structure member at the
   given offset in bytes, using the given type.  */
#define NAR_STRUCT_MEMBER(typename, ptr, offset) \
	(*NAR_NEXT(typename, ptr, offset))

/* Compatibility macro for GLib, return an untyped pointer to a
   structure member at the given offset in bytes.  */
#define NAR_STRUCT_MEMBER_P(ptr, offset) \
	NAR_NEXT(void, ptr, offset)

/* Compatibility macro for GLib, compute the byte offset of the stated
   structure field of the stated structure type.  */
#define NAR_STRUCT_OFFSET(typename, field) \
	offsetof(typename, field)

/********************************************************************/

/* Get the size contained in a size header, just before the indicated
   structure.  */
#define NAR_SIZE_HEADER(typename, ptr) \
	(*NAR_PREV_REC(typename, ptr))

/* Get the size contained in a size trailer, just after the indicated
   structure.  */
#define NAR_SIZE_TRAILER(typename, ptr) \
	(*NAR_NEXT_REC(typename, ptr))

/********************************************************************/
/* The following routines are useful for managing a traditional
   Unix-style dynamic memory allocation heap.  They may be too
   specific to be useful in other code contexts.  */

/* Navigate to previous block.  */
/* Navigate to next block.  */
/* Get the header of the current block.  */
/* Get the header of the previous block.  */
/* Get the header of the next block.  */
/* Get the trailer of the previous block.  */
/* Get the trailer of the current block.  */
/* Get the trailer of the next block.  */
/* Get the allocation status from a header.  */
/* Get the size from a header.  */

/* The main point here is that we have different types for headers and
   trailers so that readers don't get confused in our code.  */

/*

TODO: Better design is to use a more generic implementation that can
be reused for other tasks such as navigating RIFFA chunks, linked
lists, etc.  Also, it will be important to write a suite of actual
programs that use this code in the stated ways.

So, let's give this a go.

*/

/* Navigate to next pointer in a linked list, using the given macro to
   compute the next address.  The pointer in the linked lists points
   directly to the `next' pointer field, not the head of the overall
   container structure.  Use `NAR_REC_FROM_FIELD()' to obtain a
   pointer to the start of the container structure.  */
#define NAR_LNLIST_NEXT(ptr) \
	((long *)*(long *)ptr)

/* Navigate to nth item in a linked list, starting at item "start" and
   using the given "thunk" or "inner body macro" to compute each next
   address.  The result is stored in `result'.  The thunk macro must
   take the form of the `NAR_LNLIST_NEXT()' macro.  Due to the
   flexibility of using an abstract inner body macro, the linked list
   can be implemented as pointers, or as offsets, or so on.  */
#define NAR_LNLIST_NTH(result, start, thunk) \
	do { \
	long *cur = start; \
	while (cur != NULL) \
		cur = thunk(long, cur); \
	result = cur;
	} while (0);

/* TODO: Define specific structures for singly-linked and
   doubly-linked lists.  For these specific structures, we have the
   functions as defined, non-generically.  The important point in hand
   is that such code is reused at runtime.  The generics, yeah you
   still reuse code at compile-time.  But honestly, for runtime, it's
   not that much different than traditional C programmers who would
   rewrite "optimized" variants totally from scratch.  */

/* TODO: Write RIFF navigation functions on top of these macros.  */

/* TODO: Write literal packed fields data structure type, based off of
   these macros.  Yeah, slow but simple, intuitive, and easy to
   understand.  */

/* Navigate to next size block by adding the size at the current
   pointer to itself.  */
#define NAR_NEXT_SZ_BLK(ptr) \
	NAR_NEXT(long, ptr, *(long *)ptr)

/* Navigate to the next size block by adding the size at the current
   pointer to itself, plus the indicated padding.  */
#define NAR_NEXT_SZ_BLK_PAD(ptr, padding) \
	NAR_NEXT(long, ptr, *(long *)ptr + padding)

/* Navigate to the next size block by adding the size at the current
   pointer, excluding the most significant bit, to itself, plus the
   indicated padding.  */
#define NAR_NEXT_NOHI_SZ_BLK_PAD(ptr, padding) \
	NAR_NEXT(long, ptr, NAR_NO_HIBIT(*(long *)ptr) + padding)

/* Navigate to the next size block by adding the size at the current
   pointer, excluding the most significant bit, rounded up by memory
   alignment, to itself, plus the indicated padding.  */
#define NAR_NEXT_NOHI_SZ_BLK_ALIGN_PAD(ptr, padding) \
	NAR_NEXT(long, ptr, \
			 NAR_ALIGN_SIZE(NAR_NO_HIBIT(*(long *)ptr)) + padding)

#endif /* not NAVREC_H */
