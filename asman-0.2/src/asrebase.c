/* asrebase.c -- Change file management commands to use a different
   prefix for the archive root.

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
#include "cmdline.h"

char *prog_name;
char *old_prefix; unsigned old_prefix_len;
char *new_prefix; unsigned new_prefix_len;

int proc_one(int argc, char *argv[]);
int proc_two(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#define NUM_CMDS 7
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] =
		{ proc_one, proc_two, proc_two, proc_one,
		  proc_two, proc_one, proc_one };

	if (argc != 3)
	{
		printf("Usage: %s OLD-PREFIX NEW-PREFIX < OLD-CMDS > NEW-CMDS\n",
			   argv[0]);
		puts(
"Change commands operating on files in one prefix to operate on files in\n"
"another prefix.  Both prefixes must include a trailing slash.");
		return 1;
	}
	prog_name = argv[0];
	old_prefix = argv[1]; old_prefix_len = strlen(old_prefix);
	new_prefix = argv[2]; new_prefix_len = strlen(new_prefix);
	if (old_prefix[old_prefix_len-1] != '/' ||
	    new_prefix[new_prefix_len-1] != '/')
	{
		fprintf(stderr, "%s: missing trailing slash in prefix\n", argv[0]);
		return 1;
	}
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	return proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
}

/* Process a command that operates on one file argument within the
   archive root.  */
int proc_one(int argc, char *argv[])
{
	int retval = 0;
	char *orig_src, *src;
	/* Process the command line.  */
	if (argc < 2)
	{
		if (argc > 0)
			fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
		return 1;
	}

	orig_src = argv[argc-1];
	src = (char*)xmalloc(sizeof(char) *
						 (strlen(orig_src) + strlen(new_prefix) + 1));
	if (strstr(orig_src, old_prefix) == orig_src)
	{
		strcpy(src, new_prefix);
		strcat(src, orig_src + old_prefix_len);
	}
	else
	{
		fprintf(stderr, "stdin:%u: error: non-archive root command found\n",
				last_line_num);
		retval = 1; goto cleanup;
	}

	argv[argc-1] = src;
	write_cmdline(stdout, argc, argv, 1);
	argv[argc-1] = orig_src;

cleanup:
	xfree(src);
	return retval;
}

/* Process a command that operates on two file arguments within the
   archive root.  */
int proc_two(int argc, char *argv[])
{
	int retval = 0;
	char *orig_src, *orig_dest;
	char *src, *dest;
	/* Process the command line.  */
	if (argc < 3)
	{
		if (argc > 0)
			fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
		return 1;
	}
	if (!strcmp(argv[0], "ln") && !strcmp(argv[1], "-s"))
		return proc_one(argc, argv);

	orig_src = argv[argc-2];
	orig_dest = argv[argc-1];
	src = (char*)xmalloc(sizeof(char) *
						 (strlen(orig_src) + strlen(new_prefix) + 1));
	dest = (char*)xmalloc(sizeof(char) *
						  (strlen(orig_dest) + strlen(new_prefix) + 1));
	if (strstr(orig_src, old_prefix) == orig_src &&
		strstr(orig_dest, old_prefix) == orig_dest)
	{
		strcpy(src, new_prefix);
		strcpy(dest, new_prefix);
		strcat(src, orig_src + old_prefix_len);
		strcat(dest, orig_dest + old_prefix_len);
	}
	else
	{
		fprintf(stderr, "stdin:%u: error: non-archive root command found\n",
				last_line_num);
		retval = 1; goto cleanup;
	}

	argv[argc-2] = src;
	argv[argc-1] = dest;
	write_cmdline(stdout, argc, argv, 2);
	argv[argc-2] = orig_src;
	argv[argc-1] = orig_dest;

cleanup:
	xfree(src);
	xfree(dest);
	return retval;
}
