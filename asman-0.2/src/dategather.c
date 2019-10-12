/* Gathers created dates from files specified on standard input and
   writes them to standard output.  Note that this tool is
   Windows-only, mostly to work around limitations within a Windows
   environment that don't exist in a Unix-like environment.

Copyright (C) 2013 Andrew Makousky

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "misc.h"

int main(int argc, char *argv[])
{
	char *fileName = NULL;

	if (argv == 2 && (!strcmp(argv[1], "-h") ||
					  !strcmp(argv[1], "--help")))
	{
		printf("Usage: %s < DIRLIST\n", argv[0]);
		puts(
"Writes to standard output the creation dates of newline-separated\n"
"file names from standard input.");
		return 0;
	}

	/* Read a file name from standard input.  */
	while (exp_getline(stdin, &fileName) != EOF)
	{
		/* Get the file creation date.  */
		FILETIME fileTime;
		SYSTEMTIME UTCTime;
		HANDLE hFile;
		BOOL retval;
		if (feof(stdin))
			break;

		hFile = CreateFile(fileName, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "%s: could not open file: %s\n", argv[0], fileName);
			free(fileName); fileName = NULL;
			continue;
		}
		retval = GetFileTime(hFile, &fileTime, NULL, NULL);
		CloseHandle(hFile);
		if (!retval)
		{
			fprintf(stderr, "%s: could not read file time: %s\n",
					argv[0], fileName);
			free(fileName); fileName = NULL;
			continue;
		}

		/* Write the time in UTC to standard output, followed
		   by the filename.  */
		FileTimeToSystemTime(&fileTime, &UTCTime);
		printf("%d-%02d-%02d %02d:%02d:%02d UTC  %s\n",
			   UTCTime.wYear, UTCTime.wMonth, UTCTime.wDay,
			   UTCTime.wHour, UTCTime.wMinute, UTCTime.wSecond,
			   fileName);
		free(fileName); fileName = NULL;
	}
	free(fileName); fileName = NULL;

	return 0;
}
