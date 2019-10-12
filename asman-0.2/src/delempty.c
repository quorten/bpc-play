/* delempty.c -- Generate a script to delete empty directories and
   trees of such.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "bool.h"
#include "xmalloc.h"
#include "dirwalk.h"
#include "cmdline.h"

static DIR_STATUSCODE delempty_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf,
	DIR_STATUSCODE status);

int main(int argc, char *argv[])
{
	DirNode init = { ".", 1, NULL };
	pre_search_hook = NULL;
	post_search_hook = delempty_hook;
	if (argc != 1)
	{
		printf("Usage: %s > RM-CMDS\n", argv[0]);
		puts(
"Traverse the current working directory and emit commands to standard\n"
"output that delete any directory trees comprised only of directories\n"
"and no other files.  Symbolic links are not handled specially.");
		return 0;
	}
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	if (search_dir(&init) >= DIR_ERRORS)
		return 1;
	return 0;
}

static DIR_STATUSCODE delempty_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf,
	DIR_STATUSCODE status)
{
	if (status == DIR_EMPTY)
	{
		/* Output an `rmdir' command for this directory.  */
		char *pathname = dirnode_construct_path(node, filename);
		char *new_argv[] = { "rmdir", pathname };
		write_cmdline(stdout, 2, new_argv, 0);
		xfree(pathname);
		(*num_files)--;
	}
	return status;
}
