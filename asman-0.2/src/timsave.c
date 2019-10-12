/* timsave.c -- Generate a shell script that recursively reapplies
   the current time stamps to a given directory.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "bool.h"
#include "xmalloc.h"
#include "dirwalk.h"
#include "cmdline.h"

/* Should `chdir' be used in the command listing, or should longer
   paths be used instead?  */
static bool do_chdir;

static int display_help(char *progname);
static DIR_STATUSCODE timsave_t_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);
static DIR_STATUSCODE timsave_d_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);
static DIR_STATUSCODE timsave_tab_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);

int main(int argc, char *argv[])
{
	DirNode init = { ".", 1, NULL };
	pre_search_hook = timsave_t_hook;
	post_search_hook = NULL;
	do_chdir = false;
	while (*++argv)
	{
		char *c = *argv;
		if (*c != '-')
			return display_help(argv[0]);
		while (*++c)
		{
			switch (*c)
			{
			case 't': pre_search_hook = timsave_t_hook; break;
			case 'd': pre_search_hook = timsave_d_hook; break;
			case 'c': post_search_hook = chdir_hook;
				do_chdir = true; break;
			case 'p': post_search_hook = NULL;
				do_chdir = false; break;
			case 'b': pre_search_hook = timsave_tab_hook; break;
			default: return display_help(argv[0]);
			}
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
	printf("Usage: %s > TOUCH-CMDS\n", progname);
	puts(
"Traverse the current working directory and emit commands to standard\n"
"output that reapply the time stamps of the files and directories.\n"
"\n"
"Options:\n"
"  -t  Format date output for use with `-t' option to `touch'.\n"
"  -d  Format date output for use with `-d' option to `touch'.\n"
"  -c  Change directories rather than using long paths.\n"
"      Cannot be used with `-b'.\n"
"  -p  Construct long paths to avoid needing to change directories.\n"
"      Cannot be used with `-b'.\n"
"  -b  Formate date output as tab-separated columns of file names and\n"
"      dates.\n"
"\n"
"PORTABILITY NOTE: If your current year requires more than four\n"
"characters, the output of `-t' will not be compatible with\n"
"the current standards (as of 2013).");
	return 0;
}

/* Format date output for use with `-t' option to `touch'.  */
static DIR_STATUSCODE timsave_t_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	char *pathname = (do_chdir) ? xstrdup(filename) :
		dirnode_construct_path(node, filename);
	char datestr[23]; /* = "-yyyyyyyyyymmddhhmm.ss"; */
	char *new_argv[] = { "touch", "-t", datestr, pathname };
	struct tm *date = localtime(&(sbuf->st_mtime));

	date->tm_sec = (date->tm_sec >= 60) ? 59 : date->tm_sec;
	sprintf(datestr, "%d%02d%02d%02d%02d.%02d",
			1900 + date->tm_year, date->tm_mon + 1, date->tm_mday,
			date->tm_hour, date->tm_min, date->tm_sec);

	write_cmdline(stdout, 4, new_argv, 0);
	xfree(pathname);

	if (do_chdir && S_ISDIR(sbuf->st_mode))
	{
		char *cd_argv[] = { "cd", (char*)filename };
		write_cmdline(stdout, 2, cd_argv, 0);
	}

	return DIR_OK;
}

/* Format date output for use with `-d' option to `touch'.  */
static DIR_STATUSCODE timsave_d_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	char *pathname = (do_chdir) ? xstrdup(filename) :
		dirnode_construct_path(node, filename);
	char datestr[27]; /* = "-yyyyyyyyyy-mm-dd hh:mm.ss"; */
	char *new_argv[] = { "touch", "-d", datestr, pathname };
	struct tm *date = localtime(&(sbuf->st_mtime));

	date->tm_sec = (date->tm_sec >= 60) ? 59 : date->tm_sec;
	sprintf(datestr, "%d-%02d-%02d %02d:%02d:%02d",
			1900 + date->tm_year, date->tm_mon + 1, date->tm_mday,
			date->tm_hour, date->tm_min, date->tm_sec);

	write_cmdline(stdout, 4, new_argv, 0);
	xfree(pathname);

	if (do_chdir && S_ISDIR(sbuf->st_mode))
	{
		char *cd_argv[] = { "cd", (char*)filename };
		write_cmdline(stdout, 2, cd_argv, 0);
	}

	return DIR_OK;
}

/* Formate date output as tab-separated columns of file names and
   dates.  */
static DIR_STATUSCODE timsave_tab_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	char *pathname = dirnode_construct_path(node, filename);
	char datestr[27]; /* = "-yyyyyyyyyy-mm-dd hh:mm.ss"; */
	struct tm *date = localtime(&(sbuf->st_mtime));

	date->tm_sec = (date->tm_sec >= 60) ? 59 : date->tm_sec;
	sprintf(datestr, "%d-%02d-%02d %02d:%02d:%02d",
			1900 + date->tm_year, date->tm_mon + 1, date->tm_mday,
			date->tm_hour, date->tm_min, date->tm_sec);

	/* TODO: Do C-style path escaping here.  */
	printf("%s\t%s\n", pathname, datestr);
	xfree(pathname);

	return DIR_OK;
}
