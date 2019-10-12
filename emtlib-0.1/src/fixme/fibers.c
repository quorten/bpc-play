/* A cooperative multitasking implementation.

   Multitasking is achieved cooperatively, not pre-emptively, which
   means you will need to add co-opt points within your program for
   multitasking to happen.  For highly message driven code, this
   should be easy.  For code that does looping, a co-opt point needs
   to be added within every large looping construct for multitasking
   to work well.
*/
