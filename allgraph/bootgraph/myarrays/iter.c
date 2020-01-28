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

/* PLEASE NOTE: Not all specific instantiation classes need to
   implement all of these abstract methods.  For some data structures,
   implementing the method may come at a considerable performance
   penalty.  In order to discourage some types of programming patterns
   when using particular data structures, you may opt to design your
   structure so that it explicitly does not implement the slow
   operations in a base class, and then provide a derived class that
   does implement the slow methods.  */

/* Special methods to access object, GET and SET methods.  Not just
   literal access to an object address, but to also provide encoding
   and decoding as may be necessary with packed pixels, for example.
   Especially we need separate "read" and "write" methods for the sake
   of caches, and with pixels, we might only care to write without
   first reading.  */
/* N.B. For complex data, we should implement the ability to get a
   pointer and use a lock/unlock protocol.  Otherwise we copy the
   whole object unnecessarily.  In this case, if the primary object is
   literal data, we can get a pointer to the object rather than
   copying the object in full.  */
#define APITER_GET_OBJ(iter) (iter.data->d[iter.idx])
#define APITER_SET_OBJ(iter, val) iter.data->d[iter.idx] = val
#define APITER_GET_POBJ(iter) (iter.data->d + iter.idx)
#define APITER_RDLOCK_OBJ(iter)
#define APITER_WRLOCK_OBJ(iter)
#define APITER_UNLOCK_OBJ(iter)
/* But otherwise, if you really know what you are doing and are
   confident that your code has no programming errors, you can use
   SET_DIRTY directly without the lock/unlock discipline.  Or you
   could do other complex things with this set of primitives.  */
#define APITER_SET_DIRTY(iter)

#define APITER_FIRST(iter) iter.idx = 0
#define APITER_LAST(iter) iter.idx = iter.data->len - 1
#define APITER_NTH(iter, n) iter.idx = n
#define APITER_NTH_REV(iter, n) iter.idx = iter.data->len - 1 - n
#define APITER_NEXT(iter) iter.idx++
#define APITER_PREV(iter) iter.idx--
#define APITER_NEXT_N(iter, n) iter.idx += n
#define APITER_PREV_N(iter, n) iter.idx -= n

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

/* Auto-increment, auto-decrement access.  So you can emulate a stream
   in full.  N.B. Normally with auto-increment/decrement, we copy the
   data rather than read/write in place.  Because if we did, then it
   would make more sense to use the multiple-routine approach.  */
#define APITER_GET_OBJ_NEXT(iter, result) \
  result = APITER_GET_OBJ(iter); \
  APITER_NEXT(iter);  
#define APITER_SET_OBJ_NEXT(iter, val) \
  APITER_SET_OBJ(iter, val); \
  APITER_NEXT(iter);
#define APITER_GET_OBJ_PREV(iter, result) \
  result = APITER_GET_OBJ(iter); \
  APITER_PREV(iter);  
#define APITER_SET_OBJ_PREV(iter, val) \
  APITER_SET_OBJ(iter, val); \
  APITER_PREV(iter);

/* TODO: Error handling on NEXT/PREV.  What if there isn't one
   available?  Three options, either do an undefined action that will
   generally not result in erroneous behavior
   (incrementing/decrementing index without bounds checking), silently
   fail, or return an error.  If you return an error C-style, silently
   fail is the same as return an error but you ignore the error return
   value.  */

/* NOTE: Not only is this an iterator, but in fact it is a full
   programmatic memory access interface.  You can implement any kind
   of caching possible behind this interface.  Locality, Sequential
   access, random access, etc.

   You can also implement an interface to a pipe through this, and
   insert your coroutine switching in there.  */

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
