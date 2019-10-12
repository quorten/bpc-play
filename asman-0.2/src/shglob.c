/* A simple implementation of shell globbing.
   Only "MS-DOS" globbing is supported.
   Glob characters must have the high bit set.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "dirwalk.h"
#include "shglob.h"

typedef char* char_ptr;
EA_TYPE(char_ptr);

/* TODO: Add support for backslash escape sequences.  With that
   feature in place, it would not be necessary to set the high
   bit.  */

/* Returns -1 on failure to match.  On successful (partial) match,
   returns the new value of `wpat_ofs'.  */
static unsigned shglob_recurse_subnam(
	char *pattern, unsigned pattern_len, unsigned wpat_ofs,
	const char *filename, unsigned filen_ofs)
{
	bool file_matches = true;
	bool wildcard = false;
	bool dot_first = false;
	if (pattern == NULL || filename == NULL)
		return (unsigned)-1;
	if ((wpat_ofs == 0 || pattern[wpat_ofs-1] == '/') &&
		pattern[wpat_ofs] == '.')
		dot_first = true;
	/* Walk the pattern and path.  */
	for (; wpat_ofs < pattern_len &&
			 pattern[wpat_ofs] != '/' &&
			 filename[filen_ofs] != '\0'; wpat_ofs++, filen_ofs++)
	{
		/* Skip question marks.  */
		if (pattern[wpat_ofs] == (char)(0x80 | '?'))
		{ wildcard = true; continue; }
		else if (pattern[wpat_ofs] == (char)(0x80 | '*'))
		{
			unsigned new_wpat_ofs;
			wildcard = true;
			wpat_ofs++;
			if (wpat_ofs == pattern_len || pattern[wpat_ofs] == '/')
			{
				/* Consume all remaining characters since any
				   remaining characters up to the end of the file name
				   are matched.  */
				while (filename[filen_ofs] != '\0')
					filen_ofs++;
				break;
			}
			/* if (wpat_ofs > pattern_len)
				"error"; */
			/* Iterate over every possible wildcard pattern length.
			   For our purposes, it is easier to do backtracking than
			   to pass around a set of states and scan only once.  We
			   can skip handling the case of matching the very last
			   character in a file name as it is already covered by
			   the previous case.  */
			do
			{
				new_wpat_ofs = shglob_recurse_subnam(
					pattern, pattern_len, wpat_ofs, filename, filen_ofs);
				filen_ofs++;
			} while (filename[filen_ofs] != '\0' &&
					 new_wpat_ofs == (unsigned)-1);
			if (new_wpat_ofs == (unsigned)-1)
				file_matches = false;
			else
				wpat_ofs = new_wpat_ofs;
			/* Consume all remaining characters since they were
			   processed by the recursive calls.  */
			while (filename[filen_ofs] != '\0')
				filen_ofs++;
			break;
		}
		/* Verify literals match in the pattern and the path.  */
		else if (pattern[wpat_ofs] != filename[filen_ofs])
		{ file_matches = false; break; }
	}
	/* Skip trailing stars as these can be zero-length matches.  */
	if (file_matches)
	{
		while (wpat_ofs < pattern_len &&
			   pattern[wpat_ofs] == (char)(0x80 | '*'))
			wpat_ofs++;
	}
	if (filename[filen_ofs] != '\0' ||
		(wpat_ofs != pattern_len && pattern[wpat_ofs] != '/'))
		file_matches = false;
	/* Hack: We deliberately don't match "." and ".." using wildcards
	   unless there is a "." as the first character of the pattern
	   path segment.  Again, we're being more like MS-DOS wildcards
	   than Unix wildcards here by allowing matching of regular
	   files/directories with a leading dot.  */
	if (wildcard && !dot_first &&
		(!strcmp(filename, ".") || !strcmp(filename, "..")))
		file_matches = false;
	if (!file_matches)
		return (unsigned)-1;
	return wpat_ofs;
}

static void shglob_recurse_dir(
	char *pattern, unsigned pattern_len, unsigned pattern_ofs,
	DIR *dir_handle, const DirNode *dirnode, char_ptr_array *results);

static void shglob_recurse_name(
	char *pattern, unsigned pattern_len, unsigned pattern_ofs,
	const DirNode *dirnode, char *filename, char_ptr_array *results)
{
	unsigned filen_ofs = 0;
	unsigned wpat_ofs;
	if (pattern == NULL || filename == NULL || results == NULL)
		return;
	wpat_ofs = shglob_recurse_subnam(
		pattern, pattern_len, pattern_ofs, filename, filen_ofs);
	if (wpat_ofs != (unsigned)-1)
	{
		if (pattern[wpat_ofs] == '/')
		{
			/* Traverse into the directory and continue matching
			   subsequent path segments.  */
			char *old_cwd = getcwd(NULL, 0);
			DIR *subdir_handle;
			const DirNode new_node = {
				filename, strlen(filename), dirnode };
			/* Skip consecutive slashes.  */
			while (wpat_ofs < pattern_len && pattern[wpat_ofs] == '/')
				wpat_ofs++;
			if (chdir(filename) == -1)
				goto cleanup;
			subdir_handle = opendir(".");
			if (subdir_handle == NULL)
				goto cleanup;
			shglob_recurse_dir(
				pattern, pattern_len, wpat_ofs,
				subdir_handle, &new_node, results);
			closedir(subdir_handle);
		cleanup:
			/* This only works if we don't allow "..".  */
			/* if (strcmp(filename, "."))
				chdir(".."); */
			chdir(old_cwd);
			xfree(old_cwd);
			return;
		}
		/* Reconstruct the path name segments when adding the
		   argument.  */
		EA_APPEND(*results, dirnode_construct_path(dirnode, filename));
	}
}

static void shglob_recurse_dir(
	char *pattern, unsigned pattern_len, unsigned pattern_ofs,
	DIR *dir_handle, const DirNode *dirnode, char_ptr_array *results)
{
	struct dirent *de;
	if (pattern == NULL || dir_handle == NULL || results == NULL)
		return;
	/* Linearly try to match the pattern against each file in either
	   the current or root directory.  */
	/* Since our VFS `dirent' is designed to not return "." and ".."
	   in the results, we must specify them here manually so that "."
	   and ".." can get matched in globs too.  */
	shglob_recurse_name(
		pattern, pattern_len, pattern_ofs, dirnode, ".", results);
	shglob_recurse_name(
		pattern, pattern_len, pattern_ofs, dirnode, "..", results);
	while (de = readdir(dir_handle))
	{
		char *filename = de->d_name;
		shglob_recurse_name(
			pattern, pattern_len, pattern_ofs, dirnode, filename, results);
	}
}

static int shglob_cmp_func(const void *a, const void *b)
{
	const char_ptr *stra = (const char_ptr*)a;
	const char_ptr *strb = (const char_ptr*)b;
	return strcmp(*stra, *strb);
}

/* NOTE: `flags' and `errfunc' are ignored and unsupported.  */

int shglob_glob(char *pattern, int flags, void *errfunc,
				shglob_glob_t *pglob)
{
	unsigned pattern_len;
	unsigned pattern_ofs = 0;
	char *old_cwd = NULL;
	const DirNode root_node = { "", 0, NULL };
	const DirNode *dirnode = NULL;
	char_ptr_array results;
	DIR *dir_handle;
	struct dirent *de;
	if (pattern == NULL || pglob == NULL)
		return GLOB_ABORTED;
	pattern_len = strlen(pattern);
	EA_INIT(results, 16);
	if (pattern[0] == '/')
	{
		/* Change to the root directory.  */
		old_cwd = getcwd(NULL, 0);
		if (chdir("/") == -1)
		{
			xfree(old_cwd);
			return GLOB_ABORTED;
		}
		/* Skip leading slashes.  */
		while (pattern_ofs < pattern_len &&
			   pattern[pattern_ofs] == '/')
			pattern_ofs++;
		/* Assign an "empty" directory name as the root node so that
		   when we reconstruct paths, there will be a leading
		   slash.  */
		dirnode = &root_node;
	}
	dir_handle = opendir(".");
	shglob_recurse_dir(
		pattern, pattern_len, pattern_ofs,
		dir_handle, dirnode, &results);
	closedir(dir_handle);
	if (old_cwd != NULL)
	{ chdir(old_cwd); xfree(old_cwd); }
	qsort(results.d, results.len, sizeof(char_ptr), shglob_cmp_func);
	pglob->gl_pathc = results.len;
	pglob->gl_pathv = results.d;
	if (results.len == 0)
		return GLOB_NOMATCH;
	return 0;
}

void shglob_globfree(shglob_glob_t *pglob)
{
	char **data;
	unsigned len;
	unsigned i;
	if (pglob == NULL || pglob->gl_pathv == NULL)
		return;
	data = pglob->gl_pathv;
	len = pglob->gl_pathc;
	for (i = 0; i < len; i++)
		xfree(data[i]);
	xfree(data);
}
