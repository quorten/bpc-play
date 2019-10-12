/* permsave.c -- Generate a shell script that recursively reapplies
   the current permissions to a given directory.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "bool.h"
#include "xmalloc.h"
#include "dirwalk.h"
#include "cmdline.h"

/* Should `chdir' be used in the command listing, or should longer
   paths be used instead?  */
static bool do_chdir;

static int display_help(char *progname);
static DIR_STATUSCODE permsave_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);

int main(int argc, char *argv[])
{
	DirNode init = { ".", 1, NULL };
	pre_search_hook = permsave_hook;
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
	printf("Usage: %s > PERM-CMDS\n", progname);
	puts(
"Traverse the current working directory and emit commands to standard\n"
"output that reapply the permissions of the files and directories.\n"
"\n"
"Options:\n"
"  -c  Change directories rather than using long paths.\n"
"  -p  Construct long paths to avoid needing to change directories.");
	return 0;
}

static DIR_STATUSCODE permsave_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	char *pathname = (do_chdir) ? xstrdup(filename) :
		dirnode_construct_path(node, filename);
	char *new_argv[] = { NULL, NULL, pathname };

	{ /* Fill in the symbolic modes.  */
		char mode[23]; /* = "u=rwxs,g=rwxs,o=rwx,+t"; */
		char *curpos = mode;

		*curpos++ = 'u';
		*curpos++ = '=';
		if (S_IRUSR & sbuf->st_mode)
			*curpos++ = 'r';
		if (S_IWUSR & sbuf->st_mode)
			*curpos++ = 'w';
		if (S_IXUSR & sbuf->st_mode)
			*curpos++ = 'x';
		if (S_ISUID & sbuf->st_mode)
			*curpos++ = 's';
		*curpos++ = ',';

		*curpos++ = 'g';
		*curpos++ = '=';
		if (S_IRGRP & sbuf->st_mode)
			*curpos++ = 'r';
		if (S_IWGRP & sbuf->st_mode)
			*curpos++ = 'w';
		if (S_IXGRP & sbuf->st_mode)
			*curpos++ = 'x';
		if (S_ISGID & sbuf->st_mode)
			*curpos++ = 's';
		*curpos++ = ',';

		*curpos++ = 'o';
		*curpos++ = '=';
		if (S_IROTH & sbuf->st_mode)
			*curpos++ = 'r';
		if (S_IWOTH & sbuf->st_mode)
			*curpos++ = 'w';
		if (S_IXOTH & sbuf->st_mode)
			*curpos++ = 'x';

		if (S_ISVTX & sbuf->st_mode)
		{
			*curpos++ = ',';
			*curpos++ = '+';
			*curpos++ = 't';
		}

		*curpos++ = '\0';

		new_argv[0] = "chmod";
		new_argv[1] = mode;
		write_cmdline(stdout, 3, new_argv, 0);
	}

	{ /* Now write the ownership information.  */
		struct passwd *pwbuf = getpwuid(sbuf->st_uid);
		struct group *grbuf = getgrgid(sbuf->st_gid);
		unsigned pw_len = strlen(pwbuf->pw_name);
		unsigned gr_len = strlen(grbuf->gr_name);
		char *own = (char*)xmalloc(sizeof(char) *
								   (pw_len + 1 + gr_len + 1));
		strcpy(own, pwbuf->pw_name);
		own[pw_len] = ':';
		strcpy(own + pw_len + 1, grbuf->gr_name);

		new_argv[0] = "chown";
		new_argv[1] = own;
		write_cmdline(stdout, 3, new_argv, 0);
		xfree(own);
	}

	xfree(pathname);

	if (do_chdir && S_ISDIR(sbuf->st_mode))
	{
		char *cd_argv[] = { "cd", (char*)filename };
		write_cmdline(stdout, 2, cd_argv, 0);
	}

	return DIR_OK;
}
