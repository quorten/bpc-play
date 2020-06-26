/*

Okay, NEW IDEA for optimizing macro code generation, regardless
of options for bignum, narrow int, sequential, or parallel compute.

FIRST of all, BEGIN CHAIN and END CHAIN.  This where we begin
and end a pipeline of parallel operations, each of which must be
run separately.  Only because, of course, they would be of different
types.  Principally, unfold, map, and reduce.

THERE is where we have the option to define variables to pass
between the stages.  When we generate parallel code, vectors will
be used.  When we generate sequential code, scalars will be used,
since one component can be wholly computed on at a time.

INSIDE the definitions we have temp variable definitions for a single
map, for example.  Here, again, we have two generation modes.  For
parallel execute, we must initialize and destroy one temp variable
for each dimension, since they will be used simultaneously.  For
sequential execute, .

That's all there is to it!

Well, indeed I've got to do a much better job at organizing this
type of code.  But look, I see the end in sight where I can seamlessly
code in both C and OpenCL from a single source file.

Finally, all of this, of course, should be packed up nicely into a
single generics programming sub-library.  Make it real easy to write
code packed up in the right forms for parallel compute, but then
for basic testing, you can still test on a sequential computer.

HERE'S THE TRICK with begin chain and end chain.  We need to be able
to realize that if the user requested starting a chain, and we want to
compute in sequential mode, then we slice apart unfolds, maps, and
reduces into a single iteration.  The outer-most iteration component
is the chain begin/end which defines the looping.

However, for some operations like unfold and reduce, we need to have a
default "nil" value defined for each operator to initialize the
variable to, such that it would have no effect to the final result.
Examples:

* For addition unfold/reduce: zero (0)
* For multiplication unfold/reduce, one (1)
* For min() unfold/reduce: INT_MAX
* For max() unfold/reduce: INT_MIN

You get the idea.  Basically, we set to a value that is effectively an
identity with the defined operator.  We need an identity value defined
for each operator, such that when the operator is used with that and
any other arbitrary value, the result is the same arbitrary value.

Otherwise, if there is no identity value, we have to insert a
conditional to special-case handle the first item.

If we use bare unfolds, maps, and reduces, yes, then we generate our
looping primitives directly there.

*/

/* AGAIN, the design emphasis here.  If it's slow, the API should make
   it strongly discouraged or not even possible.  That way it's easy
   to write fast and efficient code when using the API properly.  */

/* OKAY, so the way to refactor this code, we pass in the dimension as
   an argument.  Without that extra layer of convenience to find out
   automatically, we do not need to make assumptions about the
   underlying type.  */

/* TODO FIXME: Move Map-Reduce into its own header file.  And define
   statistical functions and the like separately from vector math.
   And the special case of the limited overlap subset.  */

/* TODO FIXME: Every time we use MAP with temporary variables.  This
   won't work in parallel!  The temporary variables need to be defined
   in the map scope, not outside of it.  That's an optimization that
   only applies on CPUs with complex objects  */
/* Perform the indicated computation for each component.  Defined
   generically as a "map" primitive, the per-component operations may
   be executed in parallel and therefore must not depend on each
   other's results.  */
#define AB_MAP_C_SEQ(len, i, v_INIT, OPERS, v_DESTROY) \
  { \
    ABCompIdx i; \
    v_INIT \
    for (i = 0; i < len; i++) { \
      OPERS \
    } \
    v_DESTROY \
  }

#define AB_MAP_C_PAR(len, i, v_INIT, OPERS, v_DESTROY) \
  { \
    ABCompIdx i; \
    for (i = 0; i < len; i++) { \
      v_INIT \
      OPERS \
      v_DESTROY \
    } \
  }

/* CREATE SCALAR TEMP VARS, for use inside maps.  */

/* Fetch an ABNumx1 temporary variable from the object pool cache
   stack.  */
#define ab_fetch1_tmp_brnumx1(a)
/* Release an ABNumx1 temporary variable back to the object pool cache
   stack.  This must be done in a strict stack discipline: last-in,
   first-out.  */
#define ab_rel1_tmp_brnumx1(a)

/* tps = temporary pass, for passing between functional stages.  */
/* Declare the variable with the appropriate type.  */
#define ab_decl1_tps_brnumx1(a)
#define ab_fetch1_tps_brnumx1(a)
#define ab_idx1_tps_brnumx1(v, i)
#define ab_rel1_tps_brnumx1(a)

/* tmp = temp variable */

/* CREATE TEMP VARS FOR PASSING BETWEEN higher order primitives.  For
   sliced loops, this expands to a scalar.  For independent parallel
   sections, this expands to a vector.  */

/* Objects can be lazy allocated, but this is not ideal, if you can
   know in advance how many objects need to be allocated and
   pre-allocate them.  You can also free some of the unused cache.
   Releasing must be done in a strick stack discipline: last-in,
   first-out.  */

/* PLEASE NOTE: SLAB ALLOCATION is the name of the allocation
   discipline for temporary variables, especially scalars.  Vectors?
   Well we don't cache those by default, we initialize and destroy in
   parallel on the spot.  But, no need to worry about that detail, we
   abstract that away and we use the same calling convention to get
   and release a temporary variable.  */

/* LOOP SLICED VARIANT */
/* N.B. Should we really initialize and destroy in this private scope
   rather than outside the outer loop?  Yes.  To make a long
   explanation short, this is the most optimal method of stack
   discipline when the variables fit in machine registers, it clearly
   makes visible to the compiler the variables will not be used
   outside that scope, so the compiler can minimize/optimize stack
   space and register allocation.  In the event of using higher-order
   objects with expensive initialization and destruction, a temp
   variable stack should be used to optimize reuse.  This provides
   even greater optimizations by reusing a minimal number of temp
   variables of complex objects than the alternative of declaring all
   such objects in advance each for their separate uses.

   Again, our discipline is to make it easy for users to write
   efficient and optimal code and difficult or impossible to write
   inefficient code.  */
#define AB_MAP_C_SLICE(len, i, v_INIT, OPERS, v_DESTROY) \
  { \
    v_INIT \
    OPERS \
    v_DESTROY \
  }

/* Perform the indicated "combinator" computation to reduce each
   component together to a single scalar result.  For example, add
   together all component values.  Defined generically as a "reduce"
   primitive, the combinator can be run in parallel via a binary tree
   to compute in `log(n)` sequential steps.

   If the length of the vector is less than one, the result is
   undefined.

   The combinator must be associative so that parallel tree reductions
   can be executed.  Reduce is also known as "fold."  */
#define AB_REDUCE_C_SEQ(len, i, v_INIT, FIRST_ITER, COMBIN, v_DESTROY) \
  { \
    ABCompIdx i = 0; \
    v_INIT \
    FIRST_ITER \
    for (i = 1; i < len; i++) { \
      COMBIN \
    } \
    v_DESTROY \
  }

#define AB_ID_REDUCE_C_SEQ(len, i, v_INIT, SET_IDENT, COMBIN, v_DESTROY) \
  { \
    ABCompIdx i; \
    v_INIT \
    SET_IDENT \
    for (i = 0; i < len; i++) { \
      COMBIN \
    } \
    v_DESTROY \
  }

/* TODO: Generic reduce.  Depending on the compile-time selections,
   one of the specific sub-methods will be chosen, so to generalize to
   both, you must provide all arguments for either of the two
   sub-methods.  */

/* IMPORTANT for further generalization.  Is the order of iteration
   not important?  For vector data, it generally never is.  In this
   case, on scalar single-threaded processors, stepping backwards from
   the end is more register efficient than stepping forwards from the
   beginning.  */

/********************************************************************/

/* Fetch an ABNumx1 temporary variable from the object pool cache
   stack.  */
/* TODO FIXME: Right now, this code is only good for machine integers
   since it reduces down to a straight init.  */
#define ab_fetch1_tmp_brnumx1(a) \
  ab_init1_brnumx1(a)

/* Release an ABNumx1 temporary variable back to the object pool cache
   stack.  This must be done in a strict stack discipline: last-in,
   first-out.  */
/* TODO FIXME: Right now, this code is only good for machine integers
   since it reduces down to a straight destroy.  */
#define ab_rel1_tmp_brnumx1(a) \
  ab_clear1_brnumx1(a)

/********************************************************************/

/* Declare the variable with the appropriate type for a "temporary
   pass," used for passing data between higher-order vector functions
   (unfold, map, reduce).  Vector compute mode is parallel emulation
   via loop (pemu).  */
/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_decl1_tps_pemu_brnumx1(a) \
  ABVecA a

/* Fetch a temporary pass variable from the object pool cache
   stack.  */
/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_fetch1_tps_pemu_brnumx1(a, len) \
  ab_init_len2_brvec(&(a), (len)); \
  ab_init1_brvec(&(a));

/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_idx1_tps_pemu_brnumx1(v, i) \
  AB_IDX_brvec(&(v), (i))

/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_rel1_tps_pemu_brnumx1(a) \
  ab_clear1_brvec(a); \
  ab_clear_len1_brvec(a);

/********************************************************************/

/* Declare the variable with the appropriate type for a "temporary
   pass," used for passing data between higher-order vector functions
   (unfold, map, reduce).  Vector compute mode is sequential loop
   (seql).  */
/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_decl1_tps_seql_brnumx1(a) \
  ABNumx1 a

/* Fetch a temporary pass variable from the object pool cache
   stack.  */
/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_fetch1_tps_seql_brnumx1(a, len) \
  ab_init_len2_brvec(&(a), (len)); \
  ab_init1_brvec(&(a));

/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_idx1_tps_seql_brnumx1(v, i) \
  AB_IDX_brvec(&(v), (i))

/* TODO FIXME: Right now, this code is only good for machine integers
   and fixed-length vectors.  */
#define ab_rel1_tps_seql_brnumx1(a) \
  ab_clear1_brvec(a); \
  ab_clear_len1_brvec(a);
