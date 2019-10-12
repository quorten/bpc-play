/* Test that a program can modify its `argv' and that doing so causes
   the process name to appear differently to `ps'.  I know, it sounds
   like a weird thing to do, but I've been told that this is supposed
   to work.  */

/* See the COPYING file for license details.  */

#include <stdio.h>

int
main (int argc, char *argv[])
{
  int c;
  argv[0][0] = 'r';
  argv[0][1] = 'e';
  argv[0][2] = 'd';
  argv[0][3] = '\0';
  c = getchar (); /* This causes the process to sleep.  */
  return 0;
}
