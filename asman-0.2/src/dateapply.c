/* Applies dates to files specified in dirlist.txt and dates specified
   from standard input.  Note that this tool is Windows-only, mostly
   to work around limitations within a Windows environment that don't
   exist in a Unix-like environment.

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
#include <ctype.h>

#include "bool.h"
#include "misc.h"

void DateParse(const char *timeStr, FILETIME *fileTime);
void AltDateParse(const char *timeStr, FILETIME *fileTime);

int main(int argc, char *argv[])
{
	int ask_help = 0;
	char *fileName = NULL;
	char *fileTimeStr = NULL;
	FILE *dirListFile;

	ask_help = (argc == 2 && (!strcmp(argv[1], "-h") ||
							  !strcmp(argv[1], "--help")));
	if (argc != 2 || ask_help)
	{
		printf("Usage: %s DIRLIST < DATES\n", argv[0]);
		puts(
"Apply newline-separated creation dates specified on standard input to\n"
"newline-separated file names listed in DIRLIST.");
		if (ask_help)
			return 0;
		else
			return 1;
	}
	dirListFile = fopen(argv[1], "r");
	if (dirListFile == NULL)
	{
		fprintf(stderr, "%s: ", argv[0]);
		perror(argv[1]);
		return 1;
	}

	/* Get the creation date.  */
	while (exp_getline(stdin, &fileTimeStr) != EOF)
	{
		FILETIME fileTime;
		HANDLE hFile;
		if (feof(stdin))
			break;
		exp_getline(dirListFile, &fileName);

		if (isdigit((int)fileTimeStr[0]))
			DateParse(fileTimeStr, &fileTime);
		else
			AltDateParse(fileTimeStr, &fileTime);

		/* Apply the changes to our files.  */
		hFile = CreateFile(fileName, GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		SetFileTime(hFile, &fileTime, NULL, NULL);
		CloseHandle(hFile);

		free(fileName); fileName = NULL;
		free(fileTimeStr); fileTimeStr = NULL;
	}
	free(fileName); fileName = NULL;
	free(fileTimeStr); fileTimeStr = NULL;
	fclose(dirListFile);

	return 0;
}

/* Parse a UTC date in ISO 8601 format.  */
void DateParse(const char *timeStr, FILETIME *fileTime)
{
	SYSTEMTIME systemTime;
	sscanf(timeStr, "%hd-%hd-%hd %hd:%hd:%hd UTC",
		   &systemTime.wYear, &systemTime.wMonth, &systemTime.wDay,
		   &systemTime.wHour, &systemTime.wMinute, &systemTime.wSecond);
	systemTime.wDayOfWeek = 3;
	systemTime.wMilliseconds = 0;
	SystemTimeToFileTime(&systemTime, fileTime);
}

/* Parse a date that is in a format similar to the format that the
   Windows file properties dialog reports file times in.  This is how
   the date should look like: "January 08, 2013 5:38:49 AM".  */
void AltDateParse(const char *timeStr, FILETIME *fileTime)
{
	SYSTEMTIME st, lt; /* lt = localTime */
	TIME_ZONE_INFORMATION tz;
	char* monthNames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
							 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	unsigned monthLens[12] = { 7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8 };
	unsigned timeStrLen = strlen(timeStr);
	unsigned curPos = 0;
	unsigned i;

	/* Parse the month name.  */
	for (i = 0; i < 12; i++)
	{
		if (strstr(timeStr, monthNames[i]))
		{
			lt.wMonth = i + 1;
			curPos = monthLens[i] + 1;
			break;
		}
	}
	if (curPos >= timeStrLen)
		return;

	/* Parse the day, year, and time.  */
	sscanf(timeStr + curPos, "%hd, %hd %hd:%hd:%hd",
		   &lt.wDay, &lt.wYear, &lt.wHour, &lt.wMinute, &lt.wSecond);
	lt.wDayOfWeek = 3;
	lt.wMilliseconds = 0;

	/* Read either "AM" or "PM" off of the end of the string.  */
	curPos = timeStrLen;
	curPos -= 2;
	if (timeStr[curPos] == 'A')
	{
		if (lt.wHour == 12)
			lt.wHour = 0;
	}
	else
	{
		if (lt.wHour != 12)
			lt.wHour += 12;
	}

	/* Convert and store the file time.  */
	GetTimeZoneInformation(&tz);
	TzSpecificLocalTimeToSystemTime(&tz, &lt, &st);
	SystemTimeToFileTime(&st, fileTime);
}
