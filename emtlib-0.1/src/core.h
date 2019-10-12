/* Common definitions to assist with porting.  */

#include <config.h>

/* Function prototype wrappers.  These were inspired from libjpeg.  If
   the compiler doesn't support prototypes, the arguments for a
   function will be hidden.  Note that you have to use two sets of
   parenthesis around your arguments when using these macros.  */
#ifdef PROTOTYPES
#define FPW(arguments) arguments
#else
#define FPW(arguments) ()
#endif

/* Better processing?  Alright, here's what needs to be done.  Always
   keep track of comments and strings.  Also keep track of
   preprocessor defines.  Start by lexing the code, expanding
   preprocessor tokens if found.  You can set up an internal
   application pipeline and keep track of high level positions.  Once
   you do all of this processing, it is possible to keep track of the
   brace level.  When the brace level is zero, look for a series of
   tokens followed by some parenthesis.  This is a function
   definition.  Now you can simply convert the parenthesis expression
   to the non-prototype version.  Additionally, top-level forward
   declarations can also be processed using this mechanism.  */

#ifdef HAVE_BAD_MAIN
#define main core_main
#endif
