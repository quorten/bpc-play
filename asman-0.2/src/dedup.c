/* dedup.c -- Analyze a checksum listing and generate a
   "deduplication" script that replaces identical files with hard
   links.

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

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"
#include "cmdline.h"

void display_help(const char *prog_name)
{
	printf("Usage: %s < SUMS > DEDUP-SCRIPT\n", prog_name);
	puts(
"Analyze a sorted checksum listing and generate a deduplication script.\n"
"The list should be in the same format that `md5sum' or `sha*sum' generates.");
}

int main(int argc, char *argv[])
{
	char *last_sum = NULL, *cur_sum = NULL;
	unsigned sum_len = 0;

	if (argc == 2) /* "-h" || "--help" */
	{
		display_help(argv[0]);
		return 0;
	}

	{ /* Initialize.  */
		char *space_pos;
		exp_getline(stdin, &cur_sum);
		space_pos = strchr(cur_sum, ' ');
		if (space_pos == NULL)
		{
			fprintf(stderr, "%s: invalid checksum format\n", argv[0]);
			xfree(last_sum); xfree(cur_sum);
			return 1;
		}
		sum_len =  space_pos - cur_sum;
	}

	if (escape_newlines)
		fputs("NL='\n'\n", stdout);

	/* Compare the checksums and create file management commands.  */
	while (!feof(stdin))
	{
		if (last_sum != NULL && cur_sum != NULL && 
			(strncmp(last_sum, cur_sum, sum_len) > 0))
		{
			fprintf(stderr, "%s: unsorted input list\n", argv[0]);
			xfree(last_sum); xfree(cur_sum);
			return 1;
		}

		if (last_sum != NULL && !strncmp(last_sum, cur_sum, sum_len))
		{
			/* Create a hard link.  */
			char *old_name = last_sum + sum_len + 2;
			char *new_name = cur_sum + sum_len + 2;
			if (strcmp(old_name, new_name))
			{
				char *new_argv[] = { "ln", "-f", NULL, NULL };
				new_argv[2] = old_name;
				new_argv[3] = new_name;
				write_cmdline(stdout, 4, new_argv, 0);
			}
		}

		/* Advance to the next file in the list.  */
		xfree(last_sum); last_sum = cur_sum;
		exp_getline(stdin, &cur_sum);
	}
	xfree(last_sum); xfree(cur_sum);
	return 0;
}
