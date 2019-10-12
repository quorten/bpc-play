/* szsave.c -- Generate a shell script that recursively reapplies file
   sizes to regular files in a given directory.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/* If this is what it takes to get `struct stat64'...  */
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "bool.h"
#include "xmalloc.h"
#include "dirwalk.h"
#include "cmdline.h"

/* Should `chdir' be used in the command listing, or should longer
   paths be used instead?  */
static bool do_chdir;

static int display_help(char *progname);
static DIR_STATUSCODE szsave_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);

int main(int argc, char *argv[])
{
	DirNode init = { ".", 1, NULL };
	pre_search_hook = szsave_hook;
	post_search_hook = NULL;
	do_chdir = false;
	if (argc > 2)
		return display_help(argv[0]);
	if (argc > 1)
	{
		if (argv[1][0] != '-')
			return display_help(argv[0]);
		switch (argv[1][1])
		{
		case 'c': post_search_hook = chdir_hook;
			do_chdir = true; break;
		case 'p': post_search_hook = NULL;
			do_chdir = false; break;
		default: return display_help(argv[0]);
		}
	}
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	if (search_dir(&init) >= DIR_ERRORS)
		return 1;
	return 0;
}

static int display_help(char *progname)
{
	printf("Usage: %s > SZ-CMDS\n", progname);
	puts(
"Traverse the current working directory and emit commands to standard\n"
"output that set the file sizes of regular files to their current sizes.\n"
"\n"
"Options:\n"
"  -c  Change directories rather than using long paths.\n"
"  -p  Construct long paths to avoid needing to change directories.");
	return 0;
}

static DIR_STATUSCODE szsave_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	char *pathname = (do_chdir) ? xstrdup(filename) :
		dirnode_construct_path(node, filename);
	char numbuf[21]; /* 64-bit int "-9223372036854775807\0" */
	char *new_argv[] = { "truncate", "-s", numbuf, pathname };

	if (S_ISREG(sbuf->st_mode))
	{
		struct stat64 s64buf;
		if (stat64(filename, &s64buf) == 0)
		{
			sprintf(numbuf, "%lld", s64buf.st_size);
			write_cmdline(stdout, 4, new_argv, 0);
		}
		else
			numbuf[0] = '\0'; /* error */
			/* sprintf(numbuf, "%ld", sbuf->st_size); */
	}

	xfree(pathname);

	if (do_chdir && S_ISDIR(sbuf->st_mode))
	{
		char *cd_argv[] = { "cd", (char*)filename };
		write_cmdline(stdout, 2, cd_argv, 0);
	}

	return DIR_OK;
}
