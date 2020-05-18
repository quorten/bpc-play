/* Simple experiment in object-oriented polymorphism in C.  */

typedef unsigned long (*AbGetSize)(void);
typedef void (*AbInitFunc)(int*);
typedef int* (*BinArithOp)(int*, int*, int*);

struct AbScalMath_tag
{
  AbGetSize get_size;
  AbInitFunc init;
  BinArithOp add;
  BinArithOp sub;
  BinArithOp mul;
  BinArithOp div;
  BinArithOp shl;
  BinArithOp shr;
};
typedef struct AbScalMath_tag AbScalMath;

#define NUM_TYPES 1

AbScalMath g_type_vtbl[NUM_TYPES];

/* Abstract object header standard fields.  */
struct AbObj_tag
{
  int type_id;
};
typedef struct AbObj_tag AbObj;

/* Helper macro to call a virtual method.  */
#define V(obj) g_type_vtbl[((AbObj*)obj)->type_id]

/* How to use this polymorphism.  Two ways.

   1. Define your type ID in your data structure, and use that to
      index into the type vtbl in your calling code.

   2. Point directly to the type vtbl from your data structure.

   The crux of this code is that the specific functions only work with
   pointers.  Therefore, the immediate calling code need not know the
   width of the data types.  And if the width does need to be known to
   allocate temporary variables, it can be determined dynamically by
   an abstract function and then used in dynamic memory allocation.

   To use "method chaining" notation with approach #2, .  Technically
   you can use method chaining notation with approach #1, but the
   resulting code may look rather ugly, though it will be more
   efficient.  For example:

   g_type_vtbl[
     g_type_vtbl[myobj->type_id].this(myobj, ...)
               ->type_id].that(myobj, ...)

   But let's simplify with macros.

   #define V(obj) g_type_vtbl[obj->type_id]

   V(V(V(myobj).this(myobj, ...)).that(myobj, ...)).again(myobj, ...)

   Okay, that syntax is actually quite tenable.  The main requirement
   is a standard form object header that includes the type ID.

   Compare that with method #2:

   myobj->vtbl->this(myobj, ...)->vtbl->that(myobj, ...)->
     vtbl->again(myobj, ...)

   A type ID integer is better than a pointer because it can be a
   narrow integer when you don't have many types.  And, most small
   software will not have >65535 types, very small software will
   definitely not have >255 types.

   And, here's the trick.  A very smart optimizing C compiler that
   allows for "cost initialize-once" data structure fields can
   determine the results of non-polymorphic function calls at
   compile-time, and then substitute the call directly when compiling.
   And when the object address is known at compile-time, all data
   structure fields can be converted to direct addresses.

   Another great feature is defining constant data structure fields
   initialized at declaration.  But in both cases, it should be
   essential to declare such fields as "no storage" at runtime, they
   are never meant to be allocated at runtime.

   Okay, I think I really like the results.  Taking the best of
   GObject and making it better, which on its own was already better
   than libJPEG's and GNU Ghostscript's approach to object-oriented
   programming in C.

*/

/* Data structures without a C struct?  Yes, this is how it's done
   with a macro assembler.  */

/* Point3D data structure */
/* Field types */
#define SZ_X sizeof(int)
#define SZ_Y sizeof(int)
#define SZ_Z sizeof(int)
/* Field types plus padding */
#define PSZ_X SZ_X
#define PSZ_Y SZ_Y
#define PSZ_Z SZ_Z
/* Field address offsets */
#define X 0
#define Y (X + PSZ_X)
#define Z (Y + PSZ_Y)
/* Whole structure size */
#define SIZEOF_POINT3D (Z + PSZ_Z)

/* Rect data structure */
/* Field types */
#define SZ_PT1 SIZEOF_POINT3D
#define SZ_PT2 SIZEOF_POINT3D
/* Field types plus padding */
#define PSZ_PT2 SZ_PT1
#define PSZ_PT2 SZ_PT2
/* Field address offsets */
#define PT1 0
#define PT2 (PT1 + PSZ_PT1)
/* Whole structure size */
#define SIZEOF_RECT (PT2 + PSZ_PT2)

/* Okay, that's quite a handful to type, not to mention that we have a
   problem with namespacing of the data structure field names.  That
   just contributes to uglier code if there are namespace prefixes on
   field names.  Only the simplest of code that works well without
   namespace prefixes can be acceptably programmed in such an
   environment.  */
