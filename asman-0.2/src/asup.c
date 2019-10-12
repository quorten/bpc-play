/* asup.c -- Translate file management commands on an archive root
   into an update script.

Copyright (C) 2013, 2018 Andrew Makousky

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
char *prefix;
unsigned prefix_len;
char *update_cmd;

int proc_touch(int argc, char *argv[]);
int proc_one(int argc, char *argv[]);
int proc_two(int argc, char *argv[]);

/*

TODO:

New features and functions to add to my update script generator to
fully modernize it.

* Option for passing sync list to rsync, rather than rsync per touch.
  Batch together touches.  `--files-from'.

* Option for SSH to host, using connection control.
  RSYNC_CONNECT_PROG='...'

* Batch together file management commands into a single ssh/rsh
  command.

* cp, rsh, rcp, ssh, scp, rsync support

* TODO: Block-wise update commands?  The data is passed through, the
  header is rewritten.  The update command needs to be written so that
  batching together similar to rsync is also possible.

*/

int main(int argc, char *argv[])
{
#define NUM_CMDS 7
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] =
		{ proc_touch, proc_two, proc_two, proc_one,
		  proc_two, proc_one, proc_one };

	if (argc != 3)
	{
		printf("Usage: %s PREFIX UPDATE-CMD < FM-CMDS > UP-CMDS\n", argv[0]);
		puts("\n"
"Convert file management commands to be update script commands that are\n"
"parameterized for the source and destination.  PREFIX must include a\n"
"trailing slash.  UPDATE-CMD is a single argument that specifies the\n"
"command and any arguments to it for the update command used to replace\n"
"`touch' commands.");
		return 1;
	}
	prog_name = argv[0];
	prefix = argv[1];
	update_cmd = argv[2];
	prefix_len = strlen(prefix);
	if (prefix[prefix_len-1] != '/')
	{
		fprintf(stderr, "%s: missing trailing slash in prefix\n", argv[0]);
		return 1;
	}
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	return proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
}

/* Process a `touch' command.  "Touch" is the generic term for any
   kind of command that requires creating or updating a file or
   directory.  */
int proc_touch(int argc, char *argv[])
{
	char *orig_src;
	char *src, *dest;
	char *new_argv[3];

	if (argc < 2)
	{
		if (argc > 0)
			fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
		return 1;
	}

	orig_src = argv[argc-1];
	if (strstr(orig_src, prefix) == orig_src &&
		orig_src[prefix_len] != '\0')
	{
		unsigned orig_len = strlen(orig_src);
		src = (char*)xmalloc(sizeof(char) * (orig_len - prefix_len + 7 + 1));
		dest = (char*)xmalloc(sizeof(char) * (orig_len - prefix_len + 8 + 1));
		strcpy(src, "${SRC}/");
		strcpy(dest, "${DEST}/");
		strcat(src, orig_src + prefix_len);
		strcat(dest, orig_src + prefix_len);
		/* Always remove the last path component from the
		   destination.  */
		*strrchr(dest, '/') = '\0';
	}
	else
	{
		fprintf(stderr, "stdin:%u: error: non-archive root command found\n",
				last_line_num);
		return 1;
	}

	/* Replace a touch command with the update (abstract copy)
	   command.  */
	new_argv[0] = update_cmd;
	new_argv[1] = src; new_argv[2] = dest;
	write_cmdline(stdout, 3, new_argv, 2);
	xfree(src);
	xfree(dest);
	return 0;
}

/* Process a command that operates on one file argument within the
   archive root.  */
int proc_one(int argc, char *argv[])
{
	char *orig_src, *src;

	if (argc < 2)
	{
		if (argc > 0)
			fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
		return 1;
	}

	orig_src = argv[argc-1];
	if (strstr(orig_src, prefix) == orig_src)
	{
		src = (char*)xmalloc(sizeof(char) *
							 (strlen(orig_src) - prefix_len + 8 + 1));
		strcpy(src, "${DEST}/");
		strcat(src, orig_src + prefix_len);
	}
	else
	{
		fprintf(stderr, "stdin:%u: error: non-archive root command found\n",
				last_line_num);
		return 1;
	}

	argv[argc-1] = src;
	write_cmdline(stdout, argc, argv, 1);
	argv[argc-1] = orig_src;
	xfree(src);
	return 0;
}

/* Process a command that operates on two file arguments within the
   archive root.  */
int proc_two(int argc, char *argv[])
{
	bool no_tdir_opt = false;
	/* Saved iterator positions used to delete the `-T' option at a
	   point later than when it was first found.  */
	int tdir_i, tdir_j;
	char *orig_src, *orig_dest;
	char *src, *dest;

	if (argc < 3)
	{
		if (argc > 0)
			fprintf(stderr,
					"stdin:%u: error: invalid command line for `%s'\n",
					last_line_num, argv[0]);
		return 1;
	}
	if (!strcmp(argv[0], "ln") && !strcmp(argv[1], "-s"))
		return proc_one(argc, argv);

	{ /* Check for the -T option.  */
		int i;
		for (i = 0; i < argc - 2 && !no_tdir_opt; i++)
		{
			int j;
			if (argv[i][0] != '-') /* This should never happen, but... */
				continue;
			for (j = 1; argv[i][j] != 'T' && argv[i][j] != '\0'; j++)
				/* empty loop */;
			if (argv[i][j] != 'T')
				continue;

			/* else... */
			/* We found the "no target directory" option.  Our goal is
			   to remove it and it's effects before writing out the
			   command.  */
			no_tdir_opt = true;
			tdir_i = i; tdir_j = j;
			break;
		}
	}

	orig_src = argv[argc-2];
	orig_dest = argv[argc-1];
	if (strstr(orig_src, prefix) == orig_src &&
		strstr(orig_dest, prefix) == orig_dest)
	{
		src = (char*)xmalloc(sizeof(char) *
							 (strlen(orig_src) - prefix_len + 8 + 1));
		dest = (char*)xmalloc(sizeof(char) *
							  (strlen(orig_dest) - prefix_len + 8 + 1));
		strcpy(src, "${DEST}/");
		strcpy(dest, "${DEST}/");
		strcat(src, orig_src + prefix_len);
		strcat(dest, orig_dest + prefix_len);
		if (no_tdir_opt)
		{
			/* Check if the last path name components are equal.  */
			char *dest_slash_pos = strrchr(dest, '/');
			if (!strcmp(strrchr(src, '/') + 1, dest_slash_pos + 1))
			{
				int i, j;
				/* Delete the target file name from the path.  */
				*dest_slash_pos = '\0';
				/* Delete the `-T' option.  */
				i = tdir_i; j = tdir_j;
				for (; argv[i][j] != '\0'; j++)
					argv[i][j] = argv[i][j+1];
				if (j == 2)
				{
					/* Delete the empty option argument.  */
					int k;
					xfree(argv[i]);
					for (k = i; k < argc; k++)
						argv[k] = argv[k+1];
					argc--;
				}
			}
			else
			{
				fprintf(stderr,
					"stdin:%u: warning: could not eliminate `-T' option\n",
						last_line_num);
			}
		}
	}
	else
	{
		fprintf(stderr, "stdin:%u: error: non-archive root command found\n",
				last_line_num);
		return 1;
	}

	argv[argc-2] = src;
	argv[argc-1] = dest;
	write_cmdline(stdout, argc, argv, 2);
	argv[argc-2] = orig_src;
	argv[argc-1] = orig_dest;
	xfree(src);
	xfree(dest);
	return 0;
}
