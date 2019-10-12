/* A simple directory walking interface.

Copyright (C) 2013, 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/* <stdio.h> is only required for the `stdout' mention.  */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bool.h"
#include "xmalloc.h"
#include "cmdline.h"
#include "dirwalk.h"

DIR_STATUSCODE (*pre_search_hook)(const DirNode *node, unsigned *num_files,
								  const char *filename,
								  const struct stat *sbuf) = NULL;

DIR_STATUSCODE (*post_search_hook)(const DirNode *node, unsigned *num_files,
								   const char *filename,
								   const struct stat *sbuf,
								   DIR_STATUSCODE status) = NULL;

DIR_STATUSCODE search_dir(const DirNode *node)
{
	DIR_STATUSCODE retval = DIR_OK;
	DIR *dir_handle;
	unsigned num_files = 0;
	struct dirent *de;
	dir_handle = opendir(node->name);
	if (dir_handle == NULL)
		return OPEN_ERROR;
	if (chdir(node->name) == -1)
		return CHDIR_ERROR;
	while (de = readdir(dir_handle))
	{
		char *filename = de->d_name;
		struct stat sbuf;
		DIR_STATUSCODE status = DIR_OK;
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		num_files++;

		if (lstat(filename, &sbuf) == -1)
		{ retval = LSTAT_ERROR; goto cleanup; }

		if (pre_search_hook != NULL)
			status = (*pre_search_hook)(node, &num_files, filename, &sbuf);
		if (status >= DIR_ERRORS)
		{ retval = status; goto cleanup; }

		if (S_ISDIR(sbuf.st_mode))
		{
			/* Unfortunately, not all systems have `d_namlen'.  */
			/* DirNode new_node = { filename, de->d_namlen, node }; */
			DirNode new_node = { filename, strlen(filename), node };
			status = search_dir(&new_node);
			if (post_search_hook != NULL)
				status = (*post_search_hook)(node, &num_files, filename,
											 &sbuf, status);
			if (status >= DIR_ERRORS)
			{ retval = status; goto cleanup; }
		}
	}

cleanup:
	if (strcmp(node->name, "."))
		if (chdir("..") == -1)
			retval = CHDIR_ERROR;
	closedir(dir_handle);
	if (num_files == 0 && retval == DIR_OK)
		return DIR_EMPTY;
	return retval;
}

/* Construct a full path from the root directory node to the given
   directory node and append "filename" to the end of the path.  The
   returned string is dynamically allocated memory which must be
   freed.  */
char *dirnode_construct_path(const DirNode *node, const char *filename)
{
	unsigned filename_len = strlen(filename);
	char *path;
	unsigned path_len = filename_len;
	unsigned path_pos;
	const DirNode *cur_node = node;

	while (cur_node)
	{
		path_len += cur_node->name_len + 1;
		cur_node = cur_node->parent;
	}
	cur_node = node;

	path = (char*)xmalloc(sizeof(char) * (path_len + 1));
	path[path_len] = '\0';
	path_pos = path_len;

	path_pos -= filename_len;
	memcpy(&path[path_pos], filename, filename_len);

	while (cur_node && path_pos > 0)
	{
		path[--path_pos] = '/';
		path_pos -= cur_node->name_len;
		memcpy(&path[path_pos], cur_node->name, cur_node->name_len);
		cur_node = cur_node->parent;
	}

	return path;
}

DIR_STATUSCODE chdir_hook(const DirNode *node, unsigned *num_files,
						  const char *filename, const struct stat *sbuf,
						  DIR_STATUSCODE status)
{
	if (S_ISDIR(sbuf->st_mode))
	{
		char *cd_argv[] = { "cd", ".." };
		write_cmdline(stdout, 2, cd_argv, 0);
	}
	return status;
}
