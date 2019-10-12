/* nltrans.c -- Read inputs that are null-terminated from standard
   input and write out input with newlines escaped as `${NL}' and
   nulls translated to newlines.  Why this program?  I couldn't figure
   out how to do this task with Unix tools alone.

Copyright (C) 2013 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>

int main(int argc, char *argv[])
{
	int ch;
	if (argc != 1)
	{
		printf("Usage: %s < DATA > TRANS-DATA\n", argv[0]);
		puts(
"Translate standard input to standard output by replacing newlines with\n"
"`${NL}' sequences and replacing null characters with newlines.");
		return 0;
	}
	ch = getchar();
	while (ch != EOF)
	{
		switch (ch)
		{
		case '\n': fputs("${NL}", stdout); break;
		case '\0': putchar('\n');          break;
		default:   putchar(ch);            break;
		}
		ch = getchar();
	}
	return 0;
}
