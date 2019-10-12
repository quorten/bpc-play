/* Generate random calculator symbols for testing the robustness of
   the calculator.

   This file is in the public domain.  */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/* Note: We "weight" different symbols in this table by specifying
   some symbols multiple times.  */
#define NUM_SYMS 40
const char syms[NUM_SYMS] = {
  ' ', '\t', '\n',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '+', '-', '*', '/',
  '+', '-', '*', '/',
  '+', '-', '*', '/',
  '+', '-', '*', '/',
  '+', '-', '*', '/',
  '+', '-', '*', '/', '(', ')', '=',
};

int
main (int argc, char *argv[])
{
  int c;
  int numlen = 0;
  if (argc == 2)
    srand (atoi (argv[1]));
  else
    srand (time (NULL));
  do {
    /* Prevent our numbers from getting too long.  */
    do {
      c = syms[rand () % NUM_SYMS];
      if (isdigit(c)) numlen++;
      else numlen = 0;
    } while (numlen >= 10);
  } while (putchar (c) != EOF);
  return 0;
}
