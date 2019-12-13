/* Demonstrate ITERATOR API for plain arrays!!!

Public Domain 2019 Andrew Makousky

Pointer to object.  */

#include "exparray.h"

EA_TYPE(int);
/* APITER_TYPE(int_array); */

struct int_array_piter_tag
{
  /* Any literal object can be included in `data'.  Here it is a
     pointer, but it can also be direct.  Essentially an opaque
     type.  */
  int_array *data;
  unsigned idx;
};
typedef struct int_array_piter_tag int_array_piter;

/* Abstract methods.  */

/* Special methods to access object, GET and SET methods.  Not just
   literal access to an object address, but to also provide encoding
   and decoding as may be necessary with packed pixels, for example.
   Especially we need separate "read" and "write" methods for the sake
   of caches, and with pixels, we might only care to write without
   first reading.  */
#define APITER_GET_OBJ(iter) (iter.data->d[iter.idx])
#define APITER_SET_OBJ(iter, val) iter.data->d[iter.idx] = val;

#define APITER_FIRST(iter) iter.idx = 0;
#define APITER_LAST(iter) iter.idx = iter.data->len - 1;
#define APITER_NTH(iter, n) iter.idx = n;
#define APITER_NTH_REV(iter, n) iter.idx = iter.data->len - 1 - n;
#define APITER_NEXT(iter) iter.idx++;
#define APITER_PREV(iter) iter.idx--;
#define APITER_NEXT_N(iter, n) iter.idx += n;
#define APITER_PREV_N(iter, n) iter.idx -= n;

#define APITER_IS_FIRST(iter) (iter.idx == 0)
#define APITER_IS_LAST(iter) (iter.idx == iter.data->len - 1)
/* Are we in the "end-stop" region, i.e. past the end index?  */
#define APITER_IN_END_STOP(iter) (iter.idx >= iter.data->len)
#define APITER_NOT_IN_END_STOP(iter) (iter.idx < iter.data->len)
/* Are we in the "begin-stop" region, i.e. before the begin index?
   Note that for unsigned integer arithmetic, the begin stop is the
   same as the end stop, and UINT_MAX is reserved for an invalid
   index.  Which will work out just fine when we use the same integer
   type to indicate the length of the array.  */
#define APITER_IN_BEGIN_STOP(iter) (iter.idx >= iter.data->len)
#define APITER_NOT_IN_BEGIN_STOP(iter) (iter.idx < iter.data->len)
/* Java vocabulary: NOT_HAS_NEXT() and HAS_NEXT().  Shorter words, so,
   therefore, better.  But END_STOP is more Pythonic and also more
   tpyical of for-loops in the C programming language.  */
#define APITER_HAS_NEXT(iter) (iter.idx < iter.data->len - 1)
#define APITER_HAS_PREV(iter) (iter.idx > 0 && iter.idx < iter.data->len - 1)
/* Neither in begin or end stop, i.e. check that we're in the
   allocated range.  */
#define APITER_NOT_IN_STOP(iter) \
  (APITER_NOT_IN_END_STOP(iter) && APITER_NOT_IN_BEGIN_STOP(iter))

/* Get the indicated index, without changing the iterator value.  */
#define APITER_GET_IDX_FIRST(iter) 0
#define APITER_GET_IDX_LAST(iter) (iter.data->len)
#define APITER_GET_IDX(iter) iter.idx

#define APITER_INSERT_BEFORE(iter)
#define APITER_INSERT_AFTER(iter)
#define APITER_DELETE(iter)

/* TODO: Auto-increment, auto-decrement access.  So you can emulate a
   stream in full.  */

/*

Main classes for abstract methods:

* Array
* Linked List
* Packed Array, "write-through cache"
* Packed Array, "write-back cache"

Caches are essentially a high-level way to expose arrays with
transparent optimizations on accessing elements, according to some
specific algorithm.  The only primary difference is in the case of
write-back caches that require an explicit "flush" command by user
code.

For linked lists, the solution is simple.  A linked list is
principally accessed through an iterator interface.  That is, you must
set a current position and can only access the object at the current
position.

*/

/*

Color as an opaque type!

* Primary operation is to copy it, i.e. for plotting algorithms.

* Linear blending is also an important operation, i.e. for
  anti-aliased plotting.  Not applicable in low bit depths.

* Convert to grayscale, possibly applicable in low bit depths.

* Dithering, error diffusion, definitely applicable in low bit depths.

 */
