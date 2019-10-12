/* treegen.c -- Traverse a file system tree and generate shell
   commands to recreate its structure.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

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
static DIR_STATUSCODE create_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);

int main(int argc, char *argv[])
{
	DirNode init = { ".", 1, NULL };
	pre_search_hook = create_hook;
	post_search_hook = chdir_hook;
	do_chdir = true;
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
	printf("Usage: %s > CMDS\n", progname);
	puts(
"Traverse the current working directory and emit commands to standard\n"
"output that recreate the directory tree structure.  Symbolic links\n"
"are not handled specially.\n"
"\n"
"Options:\n"
"  -c  Change directories rather than using long paths.\n"
"  -p  Construct long paths to avoid needing to change directories.");
	return 0;
}

static DIR_STATUSCODE create_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	if (S_ISDIR(sbuf->st_mode))
	{
		char *pathname = (do_chdir) ? xstrdup(filename) :
			dirnode_construct_path(node, filename);
		char *mkdir_argv[] = { "mkdir", pathname };
		char *cd_argv[] = { "cd", pathname };
		write_cmdline(stdout, 2, mkdir_argv, 0);
		if (do_chdir)
			write_cmdline(stdout, 2, cd_argv, 0);
		xfree(pathname);
	}
	else
	{
		char *pathname = (do_chdir) ? xstrdup(filename) :
			dirnode_construct_path(node, filename);
		/* Assume this is a regular file.  */
		char *touch_argv[] = { "touch", pathname };
		write_cmdline(stdout, 2, touch_argv, 0);
		xfree(pathname);
	}
	return DIR_OK;
}
