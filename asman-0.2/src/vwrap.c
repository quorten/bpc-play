/* vwrap.c -- VFS wrapper functions.  Be able to recompile simple
   programs using the VFS in place of standard file system calls.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include "vfs.h"
#include "vwrap/vfs_ctx.h"
#include "vwrap/unistd.h"

int stat(const char *path, struct stat *sbuf)
{
	FSNode *file;
	if (path == NULL || sbuf == NULL)
		return -1;
	file = vfs_stat(vfs_root, vfs_cwd, path);
	if (file == NULL)
		return -1;
	sbuf->st_mode = file->delta_mode;
	if ((file->delta_mode & DM_ISDIR))
		sbuf->st_size = file->data.files.len;
	else
		sbuf->st_size = file->data.data64.len;
	sbuf->st_mtime = file->st__mtime;
	sbuf->st_blksize = 4096;
	return 0;
}

int lstat(const char *path, struct stat *sbuf)
{
	return stat(path, sbuf);
}

int chdir(const char *path)
{
	FSNode *dnode = vfs_chdir(vfs_root, vfs_cwd, path);
	if (dnode == NULL)
		return -1;
	vfs_cwd = dnode;
	return 0;
}
