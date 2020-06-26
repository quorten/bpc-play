/* Math subroutines for vectors with ABNumx1 scalar data types.  If
   this is a ".s.c" file, then it is generalized to an arbitrary
   number of dimensions.  */

/* TODO FIXME:

* Check every operator for source-destination overwrite safety.

* Make sure every temp variable is initialized and cleared.

* Double-check allocation discipline of "no solution" exceptional
  return.

* Bignum agnostic function definitions?  Here's the trick.  For wide
  data structures, every function must accept a pointer to the
  destination storage and return that pointer as the C function return
  value.  But, how do you extend this to narrow functions that return
  the value literally?  The trick is to use macros.  The primary
  function is defined as not accepting a pointer to the destination
  storage, but rather returns the value literally.  Then you have a
  wrapper macro that assigns the return value to the given storage and
  also presents it as a value as a result of the assignment operation.
  Ideally, an optimizing compiler would see that the temp variable is
  written but never read and optimize it out.  But essentially, it's
  the same optimization as writing a temp variable to use as an
  argument to a function call.

  Now, to finalize this, you need one last trick.  You need not be
  concerned about the pointer or value syntax when calling the
  function.  GMP syntax magic provides a way out of this problem, and
  fortunately, if you return a pointer, it can still be cascaded
  transparently into an input that is declared with GMP syntax
  magic.  */

/*

TODO GENERICIZE FUNCTIONS:

ab_sqrt_u64
ab_aprx_sqrt_u64
ab_aprx_sqrt_log2_u64

*/

/*

Template parameters:

* ABBool: Boolean (and also sign +1, -1, or zero) data type.

* AB_TRUE: Boolean true value.

* AB_FALSE: Boolean false value.

* ABNumx1: Scalar number type, base width, to be used as the
  components of a vector.

* ABNumx2: Scalar number type, double width, to be used as the result
  of multiplications.  Basically only special for fixed-width integer
  arithmetic.  For floating point and bignum data types, this is the
  same as ABNumx1.

* ABBitIdx: Integer data type for referencing bit indexes up to the
  width of ABNumx2, i.e. unsigned char.  Must be a built-in C integer
  data type.  Used in base 2 logarithm (shifting) calculations.

* brnumx1: Abbreviated symbol for abstract number, base width, for use
  in higher level type names, functions, etc.

* brnumx2: Abbreviated symbol for abstract number, double-width, for
  use in higher level type names, functions, etc.

* brbitidx: Abbreviated symbol for bit index integer data type, for
  use in higher level type names, functions, etc.

* ABCompIdx: Vector component index integer type, i.e. unsigned char.
  Must be a built-in C type, i.e. arbitrary precision is verboten.

* ABVecA: "Array-based" numerically indexed vector data structure.
  This is primarily used for generic per-component calculations.

* ABVecx2A: "Array-based" numerically indexed vector data structure,
  double-width.  This is primarily used for generic per-component
  calculations.

* ABVECA_LEN_DEF: Macro definition for a macro of the form
  ABVECA_LEN(v) that determines the number of components in a vector,
  i.e. vector dimension.

* ABVECX2A_LEN_DEF: Macro definition for a macro of the form
  ABVECX2A_LEN(v) that determines the number of components in a vector
  with double-width scalars, i.e. vector dimension.

* ABVecN: "Named field" (C struct) vector data structure.  Must be
  "union" type consistent with ABVecA.  This is primarily used in
  calculations that are specific to the vector dimension.

* brvec: Abbreviated symbol for abstract vector, for use in higher
  level type names, functions, etc.

* brvecx2: Abbreviated symbol for abstract vector, double-width, for
  use in higher level type names, functions, etc.

* ABPointN: "Named field" (C struct) point data structure.

* brpoint: Abbreviated symbol for abstract point, for use in higher
  level type names, functions, etc.

*/

/* Magic number value to use for "no solution."  */
#define ABNumx1_NOSOL IVINT32_MIN
#define ABNumx2_NOSOL IVINT64_MIN

/* ALLOCATION DISCIPLINE: Obviously, static allocation is easiest: all
   objects have a fixed allocation size, and therefore the sizes and
   addresses can be computed in advance, greatly easing allocation of
   variables on the stack and in global memory.

   Nevertheless, for higher-level objects, dynamic memory allocation
   is a must.  When dynamic memory allocation is being used,
   `ab_init1_brnumx1 ()` and `ab_clear1_brnumx1 ()` subroutines must
   be called in the correct locations.

   So, here is the discipline for that use.  The challenge comes with
   arguments and return values where memory management must cross
   scope boundaries.  But, there is an easy way to simplify this.  All
   function parameters must be initialized in advance.  The
   subroutines will never free a function parameter, so when possible,
   that allows both the init and the destroy to be in the same scope.
   It is function return values that are tough to manage.  Here, the
   initialization happens at the deepest part of the call stack, and
   all higher levels just pass up the already initialized object.  It
   is up to the final user at the top level scope to destroy the
   object to free the dynamically allocated memory.  This is why the
   "three-parameter" versions that use the first parameter as a
   pointer to the return value are pretty much required for
   higher-level operations, they greatly simplify memory discipline.
   Return value format is only recommended for statically allocated
   lower-level data types, although it has been carefully designed to
   still be suitable for variable types with a dynamic memory
   allocation context.

   So, one big problem sticking out here.  What if there are wrapper
   macros being used to present a "two-parameter" subroutine as a
   "three-parameter" subroutine?  The tractability is contingent upon
   proper discipline in defining these macros in relation to the
   AB_NUM_VALUE_TYPE definition.  If this is defined,
   "three-parameter" calling conventions must be wrappers to
   "two-parameter" calling conventions that initialize the return
   value themselves.  Otherwise, the "three-parameter" calling
   convention must truly rely on the caller initializing the return
   value in advance.

   Okay, so hey.  There is in fact an easy way to simplify this, then.
   Make special macro initializers.  They check the mode and determine
   if an initialization should actually be performed or they should
   just compile to a no-op.  This will greatly simplify my code by
   simply coding everything up for the "three-parameter" case with
   transparent handling of the "two-parameter" case.

   Yes, the trick is that there are two different initialization
   classes, and you call the initializer that concurs with your
   intent.

   * Caller initializes return values and passes pointer to
     subroutine.

   * Subroutine initializes return value and passes up to caller.

   Both of those cases are straight in the code, and macro tests
   handle the proper expansion.  Then that makes your code really easy
   to understand, not just by allocating and freeing, but also
   documenting the memory management discipline at the point of use.
   And I can even use typedefs to make this obvious in the function
   signature.

   So, `destroy_final_user ()`?  That's the same in both allocation
   disciplines.  Only allocation itself is different, there are two
   types.

   `caller_pre_allocate_rv ()`.
   `allocate_rv_in_situ ()`.

   My hope, finally, is that the same policy can be applied across all
   subroutines, but if it can't, token-pasting can be a way to request
   the policy for particular subroutines.

   Unfortunately, memory use now gets very hairy if you call an
   accumulation subroutine in a loop repeatedly, for example.  Here,
   every single call will do a new memory allocation, and you must
   free one of the two excess objects and replace the accumulation
   object with just one.  This is **especially** why it is not
   recommended to use allocate-your-own-return-value with higher-level
   objects.

   Okay, fine, I absolutely agree.  Let's not lead the user down a bad
   path and provide something that's not good for them.  Static
   allocation only for method #2!

   "This is intentional by design, to discourage you from using this
   allocation method that results in horrendously terrible performance
   due to allocator overhead in loops from the excess creation and
   freeing of higher-level objects."

   It means, every single time,  the code does something like this:

   ```
   FOR_EACH_COMPONENT {
     a[i] += b[i] * c[i];
   }
   ```

   A new object is allocated to store the result of `b[i] * c[i]`, and
   then another new object is allocated to store the result of `a[i] +
   (b[i] * c[i])`.  Those two excess objects must be disposed of
   before the next iteration, and all that excess allocation and
   freeing adds up to considerable overhead in these
   performance-critical inner loops.  Essentially, it's a great
   example of why garbage collected languages can easily have weird
   performance problems due to garbage collector interruptions in code
   that looks rather straightforward.

   Still, there's room for improvement.  Any temp variable used by a
   subroutine needs to be initialized and destroyed within the
   subroutine.  But if that subroutine itself is placed in a loop,
   that overhead is multiplied, even though it need not be.  It would
   be more ideal, then, if even temporary variable storage was passed
   in as an argument.

   Obviously, these are the reasons why higher-level languages can get
   quite slow when performing math.  Yeah, they do provide convenience
   features, but for the majority of the history of those languages
   they weren't very well optimized so that convenience came at a
   price.  But, it doesn't have to be that way.  A language or library
   can be both convenient and highly performant.

   Thankfully, due to my function naming convention of specifying the
   number of arguments in the function name, I can easily extend the
   repertoire of functions to include ones where the temporary
   variables have been pre-allocated.  We also want to minimize our
   use of temporary variables as much as possible.

   Okay, how about this for temporary variables?  Use a stack
   discipline to pass them through to subroutines.  Pre-allocation of
   the stack happens at the top level, each subroutine lists its
   demands of types of temp variables.  That way, I keep init and
   destroy in my code, but it is guaranteed to go through an efficient
   path of pre-allocated objects.  Slice, slab, or slub allocator.

   CHANGE MY MIND AGAIN.  Okay, considering efficiency gains from a
   slice allocator and smart use of context-specific slice allocators,
   maybe I will reconsider this.  I'll just set up the code to PREFER
   SLICE ALLOCATOR FOR TEMP VARIABLES.  A smart programmer can then
   pre-allocate the stack of temp variables and push and pop which one
   is "top" as they need, without destroying the actual objects until
   the end of the loop.

   All temp variables and callee allocated return objects are
   allocated in this fashion.  Strict stack allocate/free discipline
   is observed.

   The main and final problem, here, is the need to pass in another
   the stack context object as another argument.  Yet this is totally
   unnecessary for static allocation.  So, again, we use the magic of
   specifying N arguments for the user to pick their calling
   convention form.

   OKAY, CHANGE MY MIND AGAIN.  Yes, for temp variables, slice
   allocator all the way down you can imagine.  But for return values,
   we are very stringent about enforcing good form.  We won't require
   continuous allocation if we don't have to.  Or, in other words, we
   will forbid it where it is clearly less efficient.  */

AB_VECA_LEN_DEF;

/* FOR NARROW machine integer types: */
/* OR for GMP-style type definition magic, use this too: */
#define AB_IDX_brvec(v, i) ((v)->d[(i)])

/* FOR WIDE higher level object types, i.e. bignums: */
/* #define AB_IDX_brvec(v, i) (&(v)->d[(i)]) */

/* Declare working format pointer from API-style type, i.e. convert
   from named field C structure to array of scalars.  */
#define AB_WORK_FORM_brvec(a, b) \
  ABVecA *a = (ABVecA*)b;

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
#define AB_MAP_C_brvec(v, i, OPERS) \
  { \
    ABCompIdx i; \
    for (i = 0; i < ABVECA_LEN(v); i++) { \
      OPERS \
    } \
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
#define AB_REDUCE_C_brvec(r, v, i, COMBIN) \
  { \
    ABCompIdx i = 0; \
    ab_set2_brnumx1(r, AB_IDX_brvec(v, i)); \
    i++; \
    for (i = 1; i < ABVECA_LEN(v); i++) { \
      COMBIN \
    } \
  }

/* TODO FIXME: Double-width defs should all go in a different file to
   avoid accidental re-definition.  */
#define AB_MAP_C_brvecx2(v, i, OPERS) \
  { \
    ABCompIdx i; \
    for (i = 0; i < ABVECX2A_LEN(v); i++) { \
      OPERS \
    } \
  }
#define AB_REDUCE_C_brvecx2(r, v, i, COMBIN) \
  { \
    ABCompIdx i = 0; \
    ab_set2_brnumx2(r, AB_IDX_brvecx2(v, i)); \
    i++; \
    for (i = 1; i < ABVECX2A_LEN(v); i++) { \
      COMBIN \
    } \
  }

/* For variable-length vectors, initialize the length of a vector and
   its top-level allocation.  Individual component allocation is not
   yet initialized, unless they are static allocation size.  */
ABVecN *ab_init_len2_brvec(ABVecN *a, ABCompIdx len)
{
  /* TODO FIXME: Determine how to flexibly define semantics here.  */
  /* For fixed component count, do nothing.  */
  return a;
}

/* Initialize storage for components of a vector.  */
ABVecN *ab_init1_brvec(ABVecN *a)
{
  /* Tags: VEC-COMPONENTS */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_init1_brnumx1(AB_IDX_brvec(wa, i)); \
  );
  return a;
}

/* Initialize storage for components of a vector, and set the value to
   zero.  */
ABVecN *ab_initz1_brvec(ABVecN *a)
{
  /* Tags: VEC-COMPONENTS */
  AB_WORK_FORM_brvec(wa, a);

  AB_MAP_C_brvec(wa, i, \
    ab_initz1_brnumx1(AB_IDX_brvec(wa, i)); \
  );
  return a;
}

ABVecN *ab_clear1_brvec(ABVecN *a)
{
  /* Tags: VEC-COMPONENTS */
  AB_WORK_FORM_brvec(wa, a);

  AB_MAP_C_brvec(wa, i, \
    ab_clear1_brnumx1(AB_IDX_brvec(wa, i)); \
  );
  return a;
}

/* For variable-length vectors, destroy dynamic allocation relating to
   the length of a vector and its top-level allocation.  Individual
   component allocation is not freed by this routine, unless they are
   static allocation size.  */
ABVecN *ab_clear_len1_brvec(ABVecN *a)
{
  /* TODO FIXME: Determine how to flexibly define semantics here.  */
  /* For fixed component count, do nothing.  */
  return a;
}

ABVecN *ab_neg2_brvec(ABVecN *a, ABVecN *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_neg2_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i)); \
  );
  return a;
}

ABVecN *ab_add3_brvec(ABVecN *a, ABVecN *b, ABVecN *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);
  AB_WORK_FORM_brvec(wc, c);

  AB_MAP_C_brvec(wa, i, \
    ab_add3_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i), \
		    AB_IDX_brvec(wc, i)); \
  );
  return a;
}

ABVecN *ab_sub3_brvec(ABVecN *a, ABVecN *b, ABVecN *c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);
  AB_WORK_FORM_brvec(wc, c);

  AB_MAP_C_brvec(wa, i, \
    ab_sub3_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i), \
		    AB_IDX_brvec(wc, i)); \
  );
  return a;
}

ABVecN *ab_muldiv4_brvec_brnumx1(ABVecN *a, ABVecN *b,
				 ABNumx1 c, ABNumx1 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_muldiv4_brnumx1(AB_IDX_brvec(wa, i), \
		       AB_IDX_brvec(wb, i), \
		       c, d); \
  );
  return a;
}

/* multiply, divide, double-width constants, with logic to try to
   avoid overflows when possible while still using double-width
   intermediates

   TODO FIXME: We manipulate our `c` and `d` arguments directly
   assuming we were given a copy, but this is not the case if these
   are actually higher-level object pointers.  */
ABVecN *ab_muldiv4_brvec_brnumx2(ABVecN *a, ABVecN *b,
				 ABNumx2 c, ABNumx2 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* Check if there even is any difference between the double-width
     and single-width scalar number types.  */
  if (ABNumx1_NO_X2)
    return ab_muldiv4_brvec_brnumx1(a, b, (ABNumx1)c, (ABNumx1)d);
  /* Ensure the numerator is always positive.  */
  if (ab_sgn1_brnumx2(c) < 0)
    { ab_neg2_brnumx2(c, c); ab_neg2_brnumx2(d, d); }
  /* If `c` is too large, shift away all the low bits from both the
     numerator and the denominator, they are insignificant.  */
  if (ab_cmp2_brnumx2(c, ABNumx2_X1_THRESHOLD) >= 0) {
    ABBitIdx num_sig_bits_c = soft_fls_brnumx2(c);
    ABBitIdx num_sig_bits_d = ab_msbidx_brnumx2(d);
    ABBitIdx shr_div;
    if (num_sig_bits_c - num_sig_bits_d >=
	ABNumx2_X1_THRESHOLD_LOG2) {
      /* Fold `c / d` together before multiplying to avoid underflow
	 since `d` is insignificant.  */
      ABNumx1 c1;
      ab_div3_brnumx2(c, c, d);
      ab_init1_brnumx1(c1);
      ab_cvt_brnumx1_brnumx2(c1, c);
      AB_MAP_C_brvec(wa, i, \
	ab_mul3_brnumx1(AB_IDX_brvec(wa, i), \
			AB_IDX_brvec(wb, i), \
			c1); \
      );
      ab_clear1_brnumx1(c1);
      return a;
    }
    /* else */
    /* Since `c` is too large and `d` is significantly large, shift
       away all the low bits from both the numerator and the
       denominator because they are insignificant.  */
    shr_div = soft_fls_brnumx2(c) - ABNumx2_X1_THRESHOLD_LOG2;
    /* Avoid asymmetric two's complement behavior.  */
    if (ab_sgn1_brnumx2(d) < 0) {
      ABNumx2 t;
      ab_init1_brnumx2(t);
      ab_pow2mask2_brnumx2(t, shr_div);
      ab_add3_brnumx2(d, d, t);
      ab_clear1_brnumx2(t);
    }
    ab_shr3_brnumx2(c, c, shr_div);
    ab_shr3_brnumx2(d, d, shr_div);
  }
  /* This should never happen due to our previous logic only acting on
     this path when `d` is significantly large.  */
  /* if (ab_sgn1_brnumx2(d) == 0) {
    /\* No solution.  *\/
    return ab_nosol1_brvec(a);
  } */
  {
    ABNumx2 t;
    ab_init1_brnumx2(t);
    AB_MAP_C_brvec(wa, i, \
      ab_cvt_brnumx2_brnumx1(t, AB_IDX_brvec(wb, i)); \
      ab_mul3_brnumx2(t, t, c); \
      ab_div3_brnumx2(t, t, d); \
      ab_cvt2_brnumx1_brnumx2(AB_IDX_brvec(wa, i), t); \
    );
    ab_clear1_brnumx2(t);
  }
  return a;
}

/* multiply double-width, divide single-width, with logic to try to
   avoid overflows when possible while still using double-width
   intermediates */
ABVecN *ab_muldiv4_brvec_brnumx2_brnumx1(ABVecN *a, ABVecN *b,
					 ABNumx2 c, ABNumx1 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  /* N.B.: After some thought about numerical stability, it turns out
     I don't currently have ideas for making a better
     muldiv_brnumx2_brnumx1 than what I have for muldiv_brnumx2.
     However, the C compiler can generate more efficient code on
     32-bit CPUs if we specify we're dividing a 64-bit by a 32-bit, so
     we could just copy-paste the same code implementation for that
     paritcular purpose.  */
  BRNumx2 d2;
  ab_init1_brnumx2(d2);
  ab_cvt2_btnumx2_brnumx1(d2, d);
  ab_muldiv4_brvec_brnumx2(a, b, c, d2);
  ab_clear1_brnumx2(d2);
  return a;
}

/* shift left, divide */
ABVecN *ab_shldiv4_brvec_brnumx1(ABVecN *a, ABVecN *b,
				 ABBitIdx c, ABNumx1 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_shldiv4_brnumx1(AB_IDX_brvec(wa, i), \
		       AB_IDX_brvec(wb, i), c, d); \
  );
  return a;
}

/* multiply, shift right, with symmetric positive/negative shift
   behavior.  `q` must not be zero.  */
ABVecN *ab_mulshr4_brvec_brnumx1(ABVecN *a, ABVecN *b,
				 ABNumx1 c, ABBitIdx q)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_mulshr4_brnumx1(AB_IDX_brvec(wa, i), \
		       AB_IDX_brvec(wb, i), \
		       c, q); \
  );
  return a;
}

/* This multiply subroutine is only useful for floating point numbers
   or similar, otherwise you should use one of the `muldiv` or
   `mulshr` subroutines.  */
ABVecN *ab_mul3_brvec_brnumx1(ABVecN *a, ABVecN *b, ABNumx1 c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_mul3_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i), \
		    c); \
  );
  return a;
}

/* Multiply scalar, add vector: `a = b + c * d`

   This is only useful for floating point numbers or similar,
   otherwise you should use one of the `muldiv` or `mulshr`
   subroutines followed by an `add` subroutine.  */
ABVecN *ab_axpy4_brvec_brnumx1(ABVecN *a, ABVecN *b, ABVecN *c, ABNumx1 d)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  ABNumx1 t;
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);
  AB_WORK_FORM_brvec(wc, c);

  ab_init1_brnumx1(t);
  AB_MAP_C_brvec(wa, i, \
    ab_mul3_brnumx1(t, \
		    AB_IDX_brvec(wc, i), \
		    d); \
    ab_add3_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i), \
		    t); \
  );
  ab_clear1_brnumx1(t);
  return a;
}

/* shift left */
ABVecN *ab_shl3_brvec(ABVecN *a, ABVecN *b, ABBitIdx c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_shl3_brnumx1(AB_IDX_brvec(wa, i), \
		    AB_IDX_brvec(wb, i), \
		    c); \
  );
  return a;
}

/* shift right, with symmetric positive/negative shift behavior.  `q`
   must not be zero.  */
ABVecN *ab_shr3_brvec(ABVecN *a, ABVecN *b, ABBitIdx c)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  AB_WORK_FORM_brvec(wa, a);
  AB_WORK_FORM_brvec(wb, b);

  AB_MAP_C_brvec(wa, i, \
    ab_fsyshr3_brnumx1(AB_IDX_brvec(wa, i), \
		       AB_IDX_brvec(wb, i), \
		       c); \
  );
  return a;
}

/* dot product of two vectors */
#ifdef AB_NUM_VALUE_TYPE
ABnumx2 ab_dot2_brvec(ABVecN *b, ABVecN *c)
#else
ABnumx2 ab_dot3_brvec(ABnumx2 a, ABVecN *b, ABVecN *c)
#endif
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  ABVecx2A t;
  AB_WORK_FORM_brvec(wb, b);
  AB_WORK_FORM_brvec(wc, c);
#ifdef AB_NUM_VALUE_TYPE
  ABNumx2 a;
#endif
  ab_init_len2_brvecx2(&t, ABVECA_LEN(wb));
  ab_init1_brvecx2(&t);

  AB_MAP_C_brvecx2(&t, i, \
    ab_mul3_brnumx2_brnumx1(AB_IDX_brvecx2(&t, i), \
			    AB_IDX_brvec(wb, i), \
			    AB_IDX_brvec(wc, i)); \
  );
  AB_REDUCE_C_brvecx2(a, &t, i, \
    ab_add3_brnumx2(a, a, AB_IDX_brvecx2(&t, i)); \
  );
  ab_clear1_brvecx2(&t);
  ab_clear_len1_brvecx2(&t);
  return a;
}

/* Magnitude squared of a vector.  */
#ifdef AB_NUM_VALUE_TYPE
ABnumx2 ab_magn2q1_brvec(ABVecN *b)
#else
ABnumx2 ab_magn2q2_brvec(ABnumx2 a, ABVecN *b)
#endif
{
#ifdef AB_NUM_VALUE_TYPE
  ABNumx2 a;
#endif
  return ab_dot3_brvec(a, b, b);
}

/* TODO FIXME Dimension-specific: `iv_perpen2_v2i32()` */

/* Assign the "no solution" value ABNumx1_NOSOL to all components of
   the given vector.  */
ABVecN *ab_nosol1_brvec(ABVecN *a)
{
  /* Tags: VEC-COMPONENTS */
  AB_WORK_FORM_brvec(wa, a);

  AB_MAP_C_brvec(wa, i, \
    ab_set2_brnumx1(AB_IDX_brvec(wa, i), ABNumx1_NOSOL); \
  );
  return a;
}

/* Test if a vector is equal to the "no solution" vector.  */
ABBool ab_is_nosol1_brvec(ABVecN *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  ABBool a;
  ABVecBoolA bool_vec;
  AB_WORK_FORM_brvec(wb, b);

  /* TODO FIXME: Bad first element init.  I see, this should be
     transformed into a Map-Reduce.  */
  AB_MAP_C_brvec(a, wb, i, \
    AB_IDX_brvec(&bool_vec, i) = \
      ab_cmp2_brnumx1(AB_IDX_brvec(wb, i), ABNumx1_NOSOL) == 0; \
  );
  AB_REDUCE_C_brvec(a, wb, i, \
    a = a && AB_IDX_brvec(&bool_vec, i); \
  );
  return a;
}

/* Test if a vector is equal to zero.  */
ABBool ab_is_zero1_brvec(ABVecN *b)
{
  /* Tags: VEC-COMPONENTS, SCALAR-ARITHMETIC */
  ABBool a;
  ABVecBoolA bool_vec;
  AB_WORK_FORM_brvec(wb, b);

  /* TODO FIXME: Bad first element init.  I see, this should be
     transformed into a Map-Reduce.  */
  AB_MAP_C_brvec(a, wb, i, \
    AB_IDX_brvec(&bool_vec, i) = \
      ab_sgn1_brnumx1(AB_IDX_brvec(wb, i)) == 0; \
  );
  AB_REDUCE_C_brvec(a, wb, i, \
    a = a && AB_IDX_brvec(&bool_vec, i); \
  );
  return a;
}

/********************************************************************/
/* Operators that require a square root computation */

#ifdef AB_NUM_VALUE_TYPE
ABNumx1 ab_magnitude1_brvec(ABNumx1 a, ABVecN *b)
#else
ABNumx1 ab_magnitude2_brvec(ABNumx1 a, ABVecN *b)
#endif
{
#ifdef AB_NUM_VALUE_TYPE
  ABNumx1 a;
#endif
  ABNumx2 t;
  ab_init1_brnumx2(t);
  ab_sqrt2_u64(a, ab_magn2q2_brvec(t, b));
  ab_clear1_brnumx2(t);
  return a;
}

/* Approximate magnitude, faster but less accurate.  */
ABNumx1 ab_magn_brvec(ABVecN *a)
{
  return ab_aprx_sqrt_u64(ab_magn2q_brvec(a));
}

/* Approximate magnitude, faster but less accurate.  The base 2
   logarithm of the result is returned.  */
ABNumx1 ab_magn_log2_brvec(ABVecN *a)
{
  return ab_aprx_sqrt_log2_u64(ab_magn2q_brvec(a));
}

/* Doubly approximate vector magnitude determined using find last bit
   set and taking the max of the largest component.  The base 2
   logarithm of the result is returned.  */
ABBitIdx ab_amagn_log2_brvec(ABVecN *a)
{
  /* Tags: VEC-COMPONENTS */
  return ab_max2_u8(ab_msbidx_brnumx1(a->x),
		    ab_msbidx_brnumx1(a->y));
}

/* Vector normalization, convert to a Q16.16 fixed-point
   representation.

   If there is no solution, the resulting point's coordinates are all
   set to ABNumx1_NOSOL.  */
IVNVec2D_i32q16 *ab_normalize2_nv2i32q16_brvec(IVNVec2D_i32q16 *a,
					       ABVecN *b)
{
  ABNumx1 d = ab_magnitude_brvec(b);
  if (d == 0) {
    /* No solution.  */
    return ab_nosol1_brvec(a);
  }
  return ab_shldiv4_brvec_brnumx1(a, b, 0x10, d);
}

/* Distance between two points.  */
ABNumx1 ab_dist2_brpoint(ABPointN *a, ABPointN *b)
{
  ABVecN t;
  ab_sub3_brvec(&t, a, b);
  return ab_magnitude_brvec(&t);
}

/* Approximate distance between two points.  */
ABNumx1 ab_adist2_brpoint(ABPointN *a, ABPointN *b)
{
  ABVecN t;
  ab_sub3_brvec(&t, a, b);
  return ab_magn_brvec(&t);
}

/* Compute the length of one vector along the direction of another
   vector.

   a_along_b(A, B) = dot_product(A, B) / magnitude(B)

   If there is no solution, the return value is set to ABNumx1_NOSOL.

   TODO FIXME: Add normalized variant.
*/
ABNumx1 ab_along2_brvec(ABVecN *a, ABVecN *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  ABNumx1 result;
  ABNumx1 d = ab_magnitude_brvec(b);
  ABNumx2 t;
  ab_init1_brnumx1(result);
  if (d == 0)
    return ab_set2_brnumx1(result, ABNumx1_NOSOL);
  t = ab_dot2_brvec(a, b);
  ab_div3_brnumx1_brnumx2_brnumx1(result, t, d);
  ab_clear1_brnumx2(t);
  return result;
}

/* Project a point onto a vector.

   proj_a_on_b(A, B) = B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to ABNumx1_NOSOL.

   TODO FIXME: Add normalized variant.
*/
ABVecN *ab_proj3_brvec(ABVecN *a, ABVecN *b, ABVecN *c)
{
  ABNumx1 d;
  d = ab_magnitude_brvec(c);
  if (d == 0) {
    /* No solution.  */
    return ab_nosol1_brvec(a);
  }
  return ab_muldiv4_brvec_i64_i32(a, c, ab_dot2_brvec(b, c), d);
}

/* Eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to ABNumx1_NOSOL.

   TODO FIXME: Add normalized variant.
*/
ABVecN *ab_elim3_brvec(ABVecN *a, ABVecN *b, ABVecN *c)
{
  ABNumx1 d;
  ABVecN t;
  d = ab_magnitude_brvec(c);
  if (d == 0) {
    /* No solution.  */
    return ab_nosol1_brvec(a);
  }
  ab_muldiv4_brvec_i64_i32(&t, c, ab_dot2_brvec(b, c), d);
  ab_sub3_brvec(a, b, &t);
  return a;
}

/* Approximate eliminate one vector component from another like "A - B".

   vec_elim(A, B) =  A - B * dot_product(A, B) / magnitude(B)

   If there is no solution, the resulting point's coordinates are all
   set to ABNumx1_NOSOL.

   TODO FIXME: Add normalized variant.
*/
ABVecN *ab_aelim3_brvec(ABVecN *a, ABVecN *b, ABVecN *c)
{
  ABNumx1 d;
  ABVecN t;
  d = ab_magn_brvec(c);
  if (d == 0) {
    /* No solution.  */
    return ab_nosol1_brvec(a);
  }
  ab_muldiv4_brvec_i64_i32(&t, c, ab_dot2_brvec(b, c), d);
  ab_sub3_brvec(a, b, &t);
  return a;
}

/* Project a point onto a line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   P + proj_L

   If there is no solution, the resulting point's coordinates are all
   set to ABNumx1_NOSOL.

   TODO FIXME: Add normalized variant.
*/
ABPointN *ab_proj3_brpoint_InLine_brvec(ABPointN *a,
					   ABPointN *b,
					   IVInLine_brvec *c)
{
  ABVecN t, u;
  ab_sub3_brvec(&t, b, &c->p0);
  ab_proj3_brvec(&u, &t, &c->v);
  if (ab_is_nosol1_brvec(&u)) {
    /* No solution.  */
    return ab_nosol1_brvec(a);
  }
  ab_add3_brvec(a, &c->p0, &u);
  return a;
}

/* Shortest path distance squared from point to line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   magnitude^2(L_rel_P - proj_L);

   If there is no solution, ABNumx2_NOSOL is returned.

   TODO FIXME: Add normalized variant.
*/
ABNumx2 ab_dist2q2_brpoint_InLine_brvec(ABPointN *a, IVInLine_brvec *b)
{
  ABVecN t, u;
  ab_sub3_brvec(&t, a, &b->p0);
  ab_proj3_brvec(&u, &t, &b->v);
  if (ab_is_nosol1_brvec(&u)) {
    ABNumx2 result;
    ab_init1_brnumx2(result);
    return ab_set2_brnumx2(result, ABNumx2_NOSOL);
  }
  return ab_dist2q2_brpoint(&t, &u);
}

/* Shortest path distance from point to line.

   Let L = location vector of point,
       D = line direction vector,
       P = line location vector

   L_rel_P = L - P;
   proj_L = proj_a_on_b(L_rel_P, D);
   magnitude(L_rel_P - proj_L);

   If there is no solution, IVINT32_MIN is returned.

   TODO FIXME: Add normalized variant.
*/
ABNumx1 ab_dist2_brpoint_InLine_brvec(ABPointN *a, IVInLine_brvec *b)
{
  ABVecN t, u;
  ab_sub3_brvec(&t, a, &b->p0);
  ab_proj3_brvec(&u, &t, &b->v);
  if (ab_is_nosol1_brvec(&u))
    return IVINT32_MIN;
  return ab_dist2_brpoint(&t, &u);
}

/* Shortest path distance from point to plane (line in 2D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
ABNumx1 ab_dist2_brpoint_Eqs_brvec(ABPointN *a, IVEqs_brvec *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  ABNumx1 d = ab_magnitude_brvec(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (ABNumx1)((ab_dot2_brvec(a, &b->v) - b->offset) / d);
}

/* Approximate shortest path distance from point to plane (line in 2D).

   Let L = location vector of point,
       A = plane surface normal vector,
       d = plane offset from origin times magnitude(A),
       plane equation = Ax - d = 0

   dot_product(L, A) / magnitude(A) - d / magnitude(A)
   = (dot_product(L, A) - d) / magnitude(A)

   If there is no solution, IVINT32_MIN is returned.
*/
ABNumx1 ab_adist2_brpoint_Eqs_brvec(ABPointN *a, IVEqs_brvec *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  ABNumx1 d = ab_magn_brvec(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  return (ABNumx1)((ab_dot2_brvec(a, &b->v) - b->offset) / d);
}

/* Shortest path distance from point to plane (line in 2D), normalized
   plane surface normal vector, Q16.16 fixed-point.

   Let L = location vector of point,
       A = normalized plane surface normal vector,
       d = plane offset from origin
       plane equation = Ax - d = 0

   dot_product(L, A) - d
*/
IVint32q16 ab_dist2_brpoint_Eqs_nv2i32q16(ABPointN *a, IVEqs_nv2i32q16 *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  return (IVint32q16)
    (ab_fsyshr2_i64(ab_dot2_brvec(a, &b->v), 0x10) - b->offset);
}

/* Alternatively, rather than using a scalar `d`, you can define a
   point in the plane as a vector and subtract it from L before
   computing.

   Let L = location vector of point,
       A = plane surface normal vector,
       P = plane location vector

   L_rel_P = L - P;
   dot_product(L_rel_P, A) / magnitude(A);

   If there is no solution, IVINT32_MIN is returned.

   TODO FIXME: Add normalized variant.
*/
ABNumx1 ab_dist2_brpoint_NRay_brvec(ABPointN *a, IVNLine_brvec *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  ABNumx1 d;
  ABVecN l_rel_p;
  d = ab_magnitude_brvec(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  ab_sub3_brvec(&l_rel_p, a, &b->p0);
  return (ABNumx1)(ab_dot2_brvec(&l_rel_p, &b->v) / d);
}

/* Approximate variant of distance to point in plane, like the
   subroutine `ab_dist2_brpoint_NRay_brvec()`.

   TODO FIXME: Add normalized variant.  */
ABNumx1 ab_adist2_brpoint_NRay_brvec(ABPointN *a, IVNLine_brvec *b)
{
  /* Tags: SCALAR-ARITHMETIC */
  ABNumx1 d;
  ABVecN l_rel_p;
  d = ab_magn_brvec(&b->v);
  if (d == 0)
    return IVINT32_MIN;
  ab_sub3_brvec(&l_rel_p, a, &b->p0);
  return (ABNumx1)(ab_dot2_brvec(&l_rel_p, &b->v) / d);
}
