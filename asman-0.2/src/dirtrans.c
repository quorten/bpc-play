/* Translates recursive ls output to path-prefixed output.  The
   recursive ls output is read from standard input, and the
   path-prefixed output is written to standard output.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>

#include "exparray.h"

EA_TYPE(char);

int main(int argc, char *argv[])
{
	char_array prefix;
	int readChar;

	if (argc != 1)
	{
		printf("Usage: %s < LS-R > FILE-LIST\n", argv[0]);
		puts(
"Translate `ls -R' output to file list output, like that which\n"
"`find . -print' emits.  Due to the nature of the `ls -R' format,\n"
"file names cannot have newlines in them.");
		return 0;
	}

	/* Process the input one character at a time.  */
	EA_INIT(prefix, 16);
	readChar = getchar();
	while (readChar != EOF)
	{
		if (readChar != '\n')
		{
			/* Read the whole prefix.  */
			EA_CLEAR(prefix);
			EA_APPEND(prefix, readChar);
			readChar = getchar();
			while (readChar != EOF && readChar != ':')
			{
				EA_APPEND(prefix, readChar);
				readChar = getchar();
			}
			if (readChar == EOF)
				break;
			EA_APPEND(prefix, '/');
			EA_APPEND(prefix, '\0');
			readChar = getchar(); /* Read the newline.  */
			/* Read until the double newline.  */
			readChar = getchar();
			while (readChar != EOF)
			{
				if (readChar == '\n')
					break;
				else
				{
					fputs(prefix.d, stdout);
					while (readChar != EOF && readChar != '\n')
					{
						putchar(readChar);
						readChar = getchar();
					}
					if (readChar == EOF)
						break;
					putchar((int)'\n');
				}
				readChar = getchar();
			}
			if (readChar == EOF)
				break;
		}
		readChar = getchar();
	}

	/* Shutdown */
	EA_DESTROY(prefix);
	return 0;
}
