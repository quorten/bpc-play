/* Simple comment stripper, blank any lines starting with '*' or ';',
   and strip to end of line on occurrence of `;'.  Read from standard
   input, write to standard output.  */

#include <stdio.h>

#include "bool.h"

int
main ()
{
  bool line_start = true;
  bool strip_mode = false;
  int c;
  while ((c = getchar ()) != EOF) {
    if (c == ';') strip_mode = true;
    else if (line_start && c == '*') strip_mode = true;
    if (c == '\n') { line_start = true; strip_mode = false; }
    else line_start = false;
    if (!strip_mode) putchar (c);
  }
}
