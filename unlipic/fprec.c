/* Demonstrate issues in floating point precision when rotating an
   object 360 degrees in several small increments, several times.  */
#include <stdio.h>
#include <math.h>
#include <limits.h>

#define DEG2RAD(angle) (((double)angle) * M_PI / 180)

/* For the best loop performance optimizations, NUM_SLICES must be a
   power of two.  Otherwise, a modulo division is required to exit the
   do-while loop below.  */
#define NUM_SLICES 256
#define DEG_ANGLE ((double)360 / NUM_SLICES)
#define RAD_ANGLE DEG2RAD(DEG_ANGLE)
#define SLOWDOWN_PRINTOUT 1024
#define BLOCK_SIZE (NUM_SLICES * SLOWDOWN_PRINTOUT)
#define LOOP_AND_MASK ((NUM_SLICES * SLOWDOWN_PRINTOUT) - 1)

int
main()
{
  register double x = 1.0;
  register double y = 0.0;
  register unsigned i = 0;
  while (i < UINT_MAX && i + BLOCK_SIZE > i) {
    printf("i = %10u, x = %20.16f, y = %20.16f\r", i, x, y);
    { /* Note: Here, we use a for loop with an alternate variable to
	 make the known ahead-of-time information obvious to the
	 compiler.  The idea is that an ideal compiler would
	 optimize the loop exit condition checks to only happen at
	 "even" boundaries.  For example, if the loop is unrolled by
	 a number of times that BLOCK_SIZE is divisible by, then
	 exit condition checks are needed only at the end of the
	 unrolled loop.  */
      register unsigned i;
      for (i = 0; i < BLOCK_SIZE; i++) {
	register double new_x = x * cos(RAD_ANGLE) - y * sin(RAD_ANGLE);
	register double new_y = y * cos(RAD_ANGLE) + x * sin(RAD_ANGLE);
	x = new_x; y = new_y;
      }
    }
    i += BLOCK_SIZE;
  }
  while (i < UINT_MAX) {
    register double new_x = x * cos(RAD_ANGLE) - y * sin(RAD_ANGLE);
    register double new_y = y * cos(RAD_ANGLE) + x * sin(RAD_ANGLE);
    x = new_x; y = new_y;
    i++;
  }
  printf("\ni = %10u, x = %18.14f, y = %18.14f\n", i, x, y);
  return 0;
}

/* Wow, very interesting observation!  When I move the main body of
   code outside of `main()' into a `do_rotate()' function, the code
   performance is considerably slower.  Weird?  Weird.  */

/* Now you ask.  What about scaling?  Scaling can be checked to verify
   that it is lossless.  Specifically, if a scale operation can be
   done without any of the coordinates needing to be truncated, then a
   scale operation is lossless.  Note tnat any scale operation that is
   a multiplication by a non-repeating decimal number is guaranteed to
   not produce repeating decimals.  Of course, translations are always
   lossless.  */

/* So, good point!  You should write your software to use a "guard"
   precision and an "authoring" precision.  When data gets shifted
   into the guard precision, you warn the user about potential loss of
   precision and ask them what to do.  For the final model, the user
   must accept the prompt of throwing away any guard precision and
   stick to only the authoring precision.  Why even have guard
   precision?  It simplifies the intermediate workflow of the user.
   When the user is done with the modeling process, it is up to them
   whether they want to keep the extra precision or not.  If so, then
   the final model must be scaled up to include the extra
   precision.  */

/* Wait, how do you do lossless rotation?  Easy.  You constrain all
   rotation operations to only happen by some minimum fixed increment,
   then you can throw away all extra precision that is not needed to
   resolve angles smaller than that.

   Wait, no, here's what's happening.  Remember, you're multiplying
   numbers, so the effects are similar to scaling.  You can only scale
   so many times before you'll have to end up throwing away precision.
   Likewise, you want to set the number of bits of precision to allow
   for the case so that when you rotate a certain number of steps up
   to 360 degrees, things end up exactly as they have started out.
   This is much more general.  And yes, this does require limiting the
   precision of the sin() and cos() function outputs so as to limit
   the required precision in the result.

   What does this mean for rotating for very small angles?  It means
   that if you want some sort of continuity for such small angles, the
   only allowed computation is strict linear interpolation.  Yes, like
   translation and scaling.  Yes, from a perceptual perspective, this
   actually makes sense.  You can limit the minimum angle to half a
   degree or something like that.  */

/* Scaling.  Warn for loss of precision, and offer the option of
   narrowing the grid size.  That is, the scale factor to real
   dimensional units.  */

/* 16 bits or 32 bits?  Start by using the least number of bits of
   precision possible to represent some model.  Then, prompt the user
   if they want to promote to more bits of precision.  Allow 24 bits
   of precision too.  8 bits?  That's way too small.  The user must
   manually configure such small amounts of precision.  4 bits?  2
   bits?  1 bit?  Again, same deal.  */

/* This is why breaking scenes up into models and objects is a good
   idea.  It emphasizes practical considerations that prevent loss of
   precision in the original data.  */
