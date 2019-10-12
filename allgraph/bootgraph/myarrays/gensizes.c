/* gensizes.c -- Generate the useful stdint.h definitions for an
   unknown platform, assuming it still follows fairly common
   standards.

Public Domain 2019 Andrew Makousky

*/

#include <stdio.h>

int
main (void)
{
  int c_size = sizeof (char);
  int s_size = sizeof (short);
  int i_size = sizeof (int);
  int l_size = sizeof (long);
  puts ("#ifndef STDINT_H");
  puts ("#define STDINT_H");
  printf ("/* Size in bytes: "
	  "char = %d, short = %d, int = %d, long = %d */\n",
	  c_size, s_size, i_size, l_size);
  if (c_size == 1) {
    puts ("typedef signed char int8_t;");
    puts ("typedef unsigned char uint8_t;");
  }
  if (s_size == 2) {
    puts ("typedef short int16_t;");
    puts ("typedef unsigned short uint16_t;");
  }
  if (i_size != 4 && l_size == 4) {
    puts ("typedef long int32_t;");
    puts ("typedef unsigned long uint32_t;");
  }
  if (i_size == 4) {
    puts ("typedef int int32_t;");
    puts ("typedef unsigned int uint32_t;");
  }
  puts ("typedef long long int64_t;");
  puts ("typedef unsigned long long uint64_t;");
  puts ("#endif /* not STDINT_H */");
  return 0;
};
