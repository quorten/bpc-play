/* See the COPYING file for license details.  */

#include <stdio.h>
#include <ctype.h>

void putnum (int num);

int
main (void)
{
  unsigned char c;
  do {
    putnum (isspace (c) != 0); putchar (' ');
    putnum (isdigit (c) != 0); putchar (' ');
    putnum (isupper (c) != 0); putchar (' ');
    putnum (islower (c) != 0); putchar (' ');
    putnum (isalpha (c) != 0); putchar (' ');
    putnum (isalnum (c) != 0); putchar (' ');
    putnum (isxdigit (c) != 0); putchar (' ');
    putnum (ispunct (c) != 0); putchar (' ');
    putnum (isgraph (c) != 0); putchar (' ');
    putnum (isprint (c) != 0); putchar (' ');
    putnum (iscntrl (c) != 0); putchar (' ');
    putnum (c); putchar ('\n');
    c++;
  } while (c < 128);
  return 0;
}
