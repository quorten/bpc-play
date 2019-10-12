/* vfs.c -- Model an in-memory file system.

Copyright (C) 2017, 2018, 2019 Andrew Makousky

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

/* A word of warning: the low-level node structures are designed to be
   able to support zero-length file names and file names with slashes
   in them.  Unix-level file operators, of course, cannot support such
   file and path names.

   Another word of warning: be careful when adjusting `delta_mode': if
   you clear `DM_ISDIR', you could cause a memory leak.  If you set
   `DM_ISDIR', you could cause heap corruption and a crash.  Don't try
   to take this shortcut if you want to replace a file with a
   directory or vice versa.  Instead, delete and create nodes as
   appropriate.  */

/* <stdio.h> is only required for the `FILE' mention in `misc.h'.  */
#include <stdio.h>
#include <stdlib.h>

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"
#include "exparray.h"
#include "vfs.h"

/* Mode switch to disable sorting the files in a directory by name.
   Sorting is enabled by default because it is faster, but it can be
   disabled to speed up a "fast import."  */
bool g_vfs_sorted = true;

/* Mode switch to enable "fast insert": do not check for duplicate
   filenames within a directory during inserts/linking.  This is
   mainly only useful for building directory structures on a "fast
   import."  */
bool g_vfs_fast_insert = false;

/* LOOPING SUBROUTINE (malloc) */
/* Allocate and initialize an FSNode.  Note that passing in `name'
   causes memory ownership to pass on to the node, so use `xstrdup()'
   if necessary.  */
FSNode *fsnode_create(char *name, DeltaMode delta_mode)
{
	/* NOTE: In the interest of generality, we allow creating nodes
	   with a NULL name.  */
	FSNode *node = (FSNode*)malloc(sizeof(FSNode));
	if (node == NULL)
		return NULL;
	node->parent = NULL;
	node->name = name;
	node->delta_mode = delta_mode;
	node->other = NULL;
	if ((delta_mode & DM_ISDIR))
		EA_INIT(node->data.files, 16);
	else
	{
		node->data.data64.d = NULL;
		node->data.data64.len = 0;
	}
	node->st__mtime = 0;
	return node;
}

/* LOOPING DIRECT */
/* LOOPING SUBROUTINE (xfree, EA_DESTROY) */
/* RECURSIVE SUBROUTINE (fsnode_destroy) */
/* Free the memory with an FSNode, and recursively destroy all
   children that are no longer referenced.  */
void fsnode_destroy(FSNode *node)
{
	if (node == NULL)
		return;
	xfree(node->name);
	if ((node->delta_mode & DM_ISDIR))
	{
		/* Since we only support QDOS file system structures, we don't
		   need to worry about getting stuck in hard link directory
		   loops here.  */
		unsigned i;
		for (i = 0; i < node->data.files.len; i++)
			fsnode_destroy(node->data.files.d[i]);
		EA_DESTROY(node->data.files);
	}
	xfree(node);
}

/* LOOPING DIRECT */
/* Linear search a directory to find the index of a particular inode.
   If the desired inode does not exist in the directory,
   `(unsigned)-1' is returned.  This may be useful for unlinking a
   specific inode, but searching by name is recommended because it is
   more optimized.  */
unsigned fsnode_find_inode(FSNode *parent, FSNode *file)
{
	if (parent == NULL || file == NULL)
		return (unsigned)-1;
	if (!(parent->delta_mode & DM_ISDIR))
		return (unsigned)-1;
	{
		unsigned files_len = parent->data.files.len;
		unsigned i;
		for (i = 0; i < files_len; i++)
		{
			if (parent->data.files.d[i] == file)
				return i;
		}
	}
	return (unsigned)-1;
}

/* PERFORMANCE CRITICAL */
/* LOOPING SUBROUTINE (strcmp) */
static int fsnode_name_cmp_find(const void *key, const void *aentry)
{
	const char *key_name = (const char*)key;
	const FSNode **node = (const FSNode**)aentry;
	return strcmp(key_name, (*node)->name);
}

/* LOOPING SUBROUTINE (strcmp) */
static int fsnode_name_cmp_sort(const void *a, const void *b)
{
	const FSNode **node_a = (const FSNode**)a;
	const FSNode **node_b = (const FSNode**)b;
	return strcmp((*node_a)->name, (*node_b)->name);
}

/* PERFORMANCE CRITICAL */
/* LOOPING SUBROUTINE (bsearch_ordered, strcmp) */
/* LOOPING DIRECT */
/* Search a directory to find the index of a particular file name.  If
   the desired name does not exist in the directory, `(unsigned)-1' is
   returned.  */
unsigned fsnode_find_name(FSNode *parent, const char *name)
{
	if (parent == NULL || name == NULL)
		return (unsigned)-1;
	if (!(parent->delta_mode & DM_ISDIR))
		return (unsigned)-1;
	if (g_vfs_sorted)
	{
		FSNode **file = bsearch_ordered(
			name, parent->data.files.d, parent->data.files.len,
			sizeof(FSNode_ptr), fsnode_name_cmp_find);
		if (file == NULL)
			return (unsigned)-1;
		return file - parent->data.files.d;
	}
	else
	{
		/* Reverse linear search to find the file.  */
		unsigned i = parent->data.files.len;
		FSNode **files_d = parent->data.files.d;
		while (i > 0)
		{
			i--;
			if (!strcmp(name, files_d[i]->name))
				return i;
		}
		return (unsigned)-1;
	}
	/* return (unsigned)-1; */
}

/* LOOPING SUBROUTINE (qsort) */
/* LOOPING DIRECT */
/* RECURSIVE SUBROUTINE (fsnode_sort_names) */
/* Given an unsorted filesystem tree, recursively sort the names in
   each dnode.  */
bool fsnode_sort_names(FSNode *node)
{
	if (node == NULL)
		return false;
	if (!(node->delta_mode & DM_ISDIR))
		return false;
	qsort(node->data.files.d, node->data.files.len,
		  sizeof(FSNode_ptr), fsnode_name_cmp_sort);
	{
		unsigned i;
		unsigned files_len = node->data.files.len;
		FSNode **files_d = node->data.files.d;
		for (i = 0; i < files_len; i++)
		{
			if ((files_d[i]->delta_mode & DM_ISDIR))
				fsnode_sort_names(files_d[i]);
		}
	}
	return true;
}

/* PERFORMANCE CRITICAL */
/* LOOPING SUBROUTINE (bs_insert_pos, strcmp, EA_INSERT) */
/* LOOPING DIRECT */
/* Link an inode into a dnode.  */
bool fsnode_link(FSNode *parent, FSNode *file)
{
	bool element_exists;
	unsigned ins_pos;
	if (parent == NULL || file == NULL)
		return false;
	if (!(parent->delta_mode & DM_ISDIR))
		return false;
	if (g_vfs_sorted)
	{
		ins_pos = bs_insert_pos(
			file->name, parent->data.files.d, parent->data.files.len,
			sizeof(FSNode_ptr),
			fsnode_name_cmp_find, &element_exists);
	}
	else
	{
		element_exists = false;
		if (!g_vfs_fast_insert)
		{
			/* Check if the file already exists.  */
			unsigned i;
			const char *name = file->name;
			FSNode **files_d = parent->data.files.d;
			unsigned files_len = parent->data.files.len;
			for (i = 0; i < files_len; i++)
			{
				if (!strcmp(name, files_d[i]->name))
				{
					element_exists = true;
					break;
				}
			}
		}
		/* Linear insert at the tail of the list.  */
		ins_pos = parent->data.files.len;
	}
	/* Do not add a second file of the same name to a directory.  */
	if (element_exists)
		return false;
	EA_INSERT(parent->data.files, ins_pos, file);
	file->parent = parent;
	return true;
}

/* LOOPING SUBROUTINE (EA_REMOVE) */
/* Unlink an inode from a dnode, using the index into the parent to
   find it.  The unlinked inode is not freed by this function, so you
   should free it if it is not used anywhere else.  In other words,
   memory ownership of the inode passes on to the caller.  */
bool fsnode_unlink_index(FSNode *parent, unsigned index)
{
	if (parent == NULL || index == (unsigned)-1)
		return false;
	if (!(parent->delta_mode & DM_ISDIR))
		return false;
	parent->data.files.d[index]->parent = NULL;
	EA_REMOVE(parent->data.files, index);
	return true;
}

/* LOOPING SUBROUTINE (fsnode_create, fsnode_link, fsnode_destroy,
   xfree) */
/* Unix-like `creat'.  Memory ownership of `name' is passed to the
   node on success.  On failure, this function frees `new_name'.  */
FSNode *fsnode_ucreat(FSNode *parent, char *name, DeltaMode delta_mode)
{
	FSNode *file;
	if (parent == NULL || name == NULL)
		goto fail;
	if (!(parent->delta_mode & DM_ISDIR))
		goto fail;
	file = fsnode_create(name, delta_mode);
	if (file == NULL)
		goto fail;
	if (!fsnode_link(parent, file))
	{
		fsnode_destroy(file);
		return NULL;
	}
	return file;
fail:
	xfree(name);
	return NULL;
}

/* LOOPING SUBROUTINE (fsnode_unlink_index, fsnode_link, xfree) */
/* LOOPING DIRECT */
/* Rename/move an inode, using the index into the parent to find it,
   Memory ownership of `new_name' is passed to the node on success.
   On failure, this function frees `new_name'.  */
bool fsnode_rename_index(FSNode *parent, unsigned index,
						 FSNode *new_parent, char *new_name)
{
	FSNode *file;
	char *old_name;
	if (parent == NULL || index == (unsigned)-1 ||
		new_parent == NULL || new_name == NULL)
		goto fail;
	if (!(parent->delta_mode & DM_ISDIR) ||
		!(new_parent->delta_mode & DM_ISDIR))
		goto fail;
	file = parent->data.files.d[index];
	if ((file->delta_mode & DM_ISDIR))
	{
		/* Do not allow moving a directory into any subdirectory
		   within itself.  */
		FSNode *ppdnode = new_parent;
		while (ppdnode != NULL)
		{
			if (ppdnode == file)
				goto fail;
			ppdnode = ppdnode->parent;
		}
	}
	if (!fsnode_unlink_index(parent, index))
		goto fail;
	old_name = file->name;
	file->name = new_name;
	if (!fsnode_link(new_parent, file))
	{
		file->name = old_name;
		/* We have to undo our previous unlink to remain in a
		   consistent state.  */
		fsnode_link(parent, file);
		goto fail;
	}
	xfree(old_name);
	return true;
fail:
	xfree(new_name);
	return false;
}

/********************************************************************/

/* PERFORMANCE CRITICAL */
/* LOOPING SUBROUTINE (strcmp, fsnode_find_name) */
/* LOOPING DIRECT */
/* Take a Unix-style path string where the directory segments are
   separated with slashes, traverse the directory tree accordingly,
   and return the destination directory.  If there is an error such as
   trying to traverse a non-directory or a directory name is not
   found, NULL is returned.  Note that this function modifies the
   `path' argument to temporarily replace slash characters with null
   characters to ease string processing.  */
FSNode *fsnode_traverse(FSNode *parent, char *path)
{
	FSNode *file = parent;
	char *end;
	if (parent == NULL)
		return NULL;
	if (path == NULL)
		return file;
	if (!(file->delta_mode & DM_ISDIR))
		return NULL;
	/* Skip leading slashes.  */
	while (*path == '/')
		path++;
	end = path;
	while (/* file != NULL && */ *path)
	{
		while (*end != '\0' && *end != '/')
			end++;
		STR_BEGIN_SPLICE(end);
		if (*path == '\0')
			; /* Skip zero-length path segment, i.e. consecutive
			     leading or trailing slash.  */
		else if (!strcmp(path, "."))
			; /* skip */
		else if (!strcmp(path, ".."))
		{
			FSNode *new_file = file->parent;
			if (new_file != NULL)
				file = new_file;
		}
		else
		{
			unsigned index = fsnode_find_name(file, path);
			if (index == (unsigned)-1)
				file = NULL;
			else
				file = file->data.files.d[index];
		}
		STR_END_SPLICE();
		if (file == NULL)
			break;
		if (*end != '\0')
		{
			/* (*end == '/') */
			end++;
			/* Paths with trailing slashes must always correspond to
			   directories.  */
			if (!(file->delta_mode & DM_ISDIR))
				return NULL;
		}
		path = end;
	}
	return file;
}

/* LOOPING SUBROUTINE (xstrdup, xmalloc, strncpy, fsnode_traverse,
   xfree) */
/* LOOPING DIRECT */
/* Higher-level wrapper function to traverse a path into a
   dnode/inode, taking into account the current working directory.
   `len' specifies a leading subset of `path' to process.  If zero,
   use the entire string in `path'.  */
FSNode *vfs_traverse(FSNode *vfs_root, FSNode *vfs_cwd,
					 const char *path, unsigned len)
{
	FSNode *parent, *dnode;
	char *temp;
	if (path == NULL)
		return NULL;
	if (!len)
		temp = xstrdup(path);
	else
	{
		temp = (char*)xmalloc(sizeof(char) * (len + 1));
		strncpy(temp, path, len);
		temp[len] = '\0';
	}
	if (temp[0] == '/')
		parent = vfs_root;
	else
		parent = vfs_cwd;
	dnode = fsnode_traverse(parent, temp);
	xfree(temp);
	return dnode;
}

/* LOOPING SUBROUTINE (strrchr) */
/* Take a path string and compute the base name of the file, using the
   same method as `vfs_dnode_basename()'.  Returns NULL on
   failure.  */
const char *vfs_basename(const char *pathname)
{
	const char *dirname_end;
	if (pathname == NULL)
		return NULL;
	dirname_end = strrchr(pathname, '/');
	if (dirname_end == NULL)
	{
		/* Easy, the entire string is the base name.  */
		return pathname;
	}
	return dirname_end + 1;
}

/* LOOPING SUBROUTINE (strrchr, vfs_traverse) */
/* Take a path string and compute the base name of the file and the
   dnode of its parent directory, using the same method as
   `vfs_basename()'.  The returned name is a pointer into the last
   segment of `pathname'.  On failure, both `parent' and `name' will
   be NULL.

   This function breaks up the basename and dirname by a very specific
   rule: the dirname is always the entire path leading up to the last
   slash.  Thus, if there is a path ending with a trailing slash, the
   dirname is the portion up to the last slash, and the base name is
   the empty string, i.e. a single null character.  This behavior
   guarantees tha paths ending with slashes will always be treated as
   existing directories.  */
VFSDnodeName vfs_dnode_basename(FSNode *vfs_root, FSNode *vfs_cwd,
								const char *pathname)
{
	VFSDnodeName result = { NULL, NULL };
	const char *dirname_end;
	unsigned dirname_len;
	if (pathname == NULL)
		return result;
	dirname_end = strrchr(pathname, '/');
	if (dirname_end == NULL)
	{
		/* Easy, the dnode is the current directory, and the entire
		   string is the base name.  */
		result.parent = vfs_cwd;
		result.name = pathname;
		return result;
	}
	if (dirname_end == pathname)
	{
		/* The dnode is the root directory and the string excluding
		   the single leading slash is the base name.  */
		result.parent = vfs_root;
		result.name = pathname + 1;
		return result;
	}
	dirname_len = dirname_end - pathname;
	result.parent = vfs_traverse(vfs_root, vfs_cwd, pathname, dirname_len);
	if (result.parent == NULL)
		return result;
	result.name = dirname_end + 1;
	return result;
}

/* LOOPING SUBROUTINE (strlen) */
/* LOOPING DIRECT */
/* Replace trailing slashes with null characters.  Return the number
   of slashes stripped.  */
unsigned strip_trail_slash(char *path)
{
	unsigned path_len = strlen(path);
	unsigned i = path_len;
	while (i > 0 && path[i-1] == '/')
		path[--i] = '\0';
	return path_len - i;
}

/********************************************************************/

/* DUPLICATE CODE */
/* LOOPING SUBROUTINE (strcmp, fsnode_find_name, fsnode_ucreat) */
/* LOOPING DIRECT */
/* Like `mkdir -p': Take a Unix-style path string where the directory
   segments are separated with slashes, traverse the directory tree
   accordingly, and create directories when they do not already exist.
   If there is an error such as trying to traverse a non-directory or
   a directory name is not found, NULL is returned.  Note that this
   function modifies the `path' argument to temporarily replace slash
   characters with null characters to ease string processing.

   On failure, the newly created directories for the earlier segments
   are not removed.

   Interestingly, this is designed to create intermediate directories
   referenced in a relative path such as "one/two/../three".

   Also, note that this function doesn't check that the final
   mentioned path segment is actually a directory.  It could just as
   well be an existing file and that would be okay.  */
FSNode *fsnode_mkdirp(FSNode *parent, char *path, DeltaMode delta_mode)
{
	FSNode *file = parent;
	char *end;
	if (parent == NULL)
		return NULL;
	if (path == NULL)
		return file;
	if (!(file->delta_mode & DM_ISDIR))
		return NULL;
	delta_mode |= DM_ISDIR;
	/* Skip leading slashes.  */
	while (*path == '/')
		path++;
	end = path;
	while (/* file != NULL && */ *path)
	{
		while (*end != '\0' && *end != '/')
			end++;
		STR_BEGIN_SPLICE(end);
		if (*path == '\0')
			; /* Skip zero-length path segment, i.e. consecutive
			     leading or trailing slash.  */
		else if (!strcmp(path, "."))
			; /* skip */
		else if (!strcmp(path, ".."))
		{
			FSNode *new_file = file->parent;
			if (new_file != NULL)
				file = new_file;
		}
		else
		{
			unsigned index = fsnode_find_name(file, path);
			if (index == (unsigned)-1)
			{
				/* Attempt to create this missing directory segment.  */
				file = fsnode_ucreat(file, xstrdup(path), delta_mode);
			}
			else
				file = file->data.files.d[index];
		}
		STR_END_SPLICE();
		if (file == NULL)
			break;
		if (*end != '\0')
		{
			/* (*end == '/') */
			end++;
			/* Paths with trailing slashes must always correspond to
			   directories.  */
			if (!(file->delta_mode & DM_ISDIR))
				return NULL;
		}
		path = end;
	}
	return file;
}

/********************************************************************/

/* LOOPING SUBROUTINE (fsnode_create, xstrdup) */
/* Create the root node of a VFS tree.  */
FSNode *vfs_create(void)
{
	return fsnode_create(xstrdup(""), DM_ISDIR);
}

/* LOOPING SUBROUTINE (fsnode_destroy) */
/* Destroy a VFS tree, freeing up all associated memory.  */
void vfs_destroy(FSNode *vfs_root)
{
	fsnode_destroy(vfs_root);
}

/* LOOPING SUBROUTINE (vfs_traverse) */
/* As far as we're concerned, a VFS `stat()' is the same thing as
   traversing a path to get the associated inode.  Therefore, we do
   not have a corresponding `vfs_fstat()'.  DO NOT attempt to free the
   returned inode!  */
FSNode *vfs_stat(FSNode *vfs_root, FSNode *vfs_cwd, const char *path)
{
	return vfs_traverse(vfs_root, vfs_cwd, path, 0);
}

FSNode *vfs_fchdir(FSNode *dnode)
{
	if (dnode == NULL || !(dnode->delta_mode & DM_ISDIR))
		return NULL;
	return dnode;
}

/* LOOPING SUBROUTINE (vfs_traverse) */
FSNode *vfs_chdir(FSNode *vfs_root, FSNode *vfs_cwd, const char *path)
{
	return vfs_fchdir(vfs_traverse(vfs_root, vfs_cwd, path, 0));
}

/* LOOPING SUBROUTINE (vfs_dnode_basename, fsnode_ucreat, strcmp,
   xstrdup) */
int vfs_ucreat(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *pathname, DeltaMode delta_mode)
{
	VFSDnodeName parts;
	char *name;

	{ /* Do not allow trailing slashes since (1) these must not appear
		 in paths for regular files and (2) these must have been
		 removed in advance for directories.  */
		unsigned path_len = strlen(pathname);
		if (path_len > 0 && pathname[path_len-1] == '/')
			return -1;
	}

	parts = vfs_dnode_basename(vfs_root, vfs_cwd, pathname);
	if (parts.parent == NULL)
		return -1;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		return -1;
	name = xstrdup(parts.name);
	if (fsnode_ucreat(parts.parent, name, delta_mode) == NULL)
		return -1;
	return 0;
}

int vfs_ftruncate(FSNode *file, FSint64 length)
{
	if (file == NULL || (file->delta_mode & DM_ISDIR))
		return -1;
	file->data.data64.len = length;
	return 0;
}

/* LOOPING SUBROUTINE (vfs_traverse) */
int vfs_truncate(FSNode *vfs_root, FSNode *vfs_cwd,
				 const char *path, FSint64 length)
{
	return vfs_ftruncate(vfs_traverse(vfs_root, vfs_cwd, path, 0),
						 length);
}

int vfs_futime(FSNode *file, FSint64 *times)
{
	if (file == NULL || times == NULL)
		return -1;
	file->st__mtime = times[0];
	return 0;
}

/* LOOPING SUBROUTINE (vfs_traverse) */
int vfs_utime(FSNode *vfs_root, FSNode *vfs_cwd,
			  const char *path, FSint64 *times)
{
	return vfs_futime(vfs_traverse(vfs_root, vfs_cwd, path, 0),
					  times);
}

/* LOOPING SUBROUTINE (xstrdup, strip_trail_slash, vfs_dnode_basename,
   fsnode_find_name, strcmp, fsnode_rename_index, xfree) */
int vfs_rename(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *oldpath, const char *newpath)
{
	char *strip_oldpath = NULL, *strip_newpath = NULL;
	unsigned src_trail_slashes, dest_trail_slashes;
	unsigned src_index;
	VFSDnodeName parts;
	VFSDnodeName new_parts;
	char *new_name;
	if (oldpath == NULL || newpath == NULL)
		return -1;

	strip_oldpath = xstrdup(oldpath);
	strip_newpath = xstrdup(newpath);
	src_trail_slashes = strip_trail_slash(strip_oldpath);
	dest_trail_slashes = strip_trail_slash(strip_newpath);
	oldpath = strip_oldpath;
	newpath = strip_newpath;

	parts = vfs_dnode_basename(vfs_root, vfs_cwd, oldpath);
	if (parts.parent == NULL)
		goto fail;
	src_index = fsnode_find_name(parts.parent, parts.name);
	if (src_index == (unsigned)-1)
		goto fail;

	{ /* If there are trailing slashes but the source is not a
		 directory, fail.  */
		FSNode *file = parts.parent->data.files.d[src_index];
		if ((src_trail_slashes != 0 || dest_trail_slashes != 0) &&
			(file->delta_mode & DM_ISDIR) == 0)
			goto fail;
	}

	/* Do not allow usage of the Unix reserved names "." and "..".
	   Even though Unix (sometimes) allows it, we don't allow moving
	   parent directories of the current working directory.  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		goto fail;
	new_parts = vfs_dnode_basename(vfs_root, vfs_cwd, newpath);
	if (new_parts.parent == NULL)
		goto fail;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(new_parts.name, ".") || !strcmp(new_parts.name, ".."))
		goto fail;
	new_name = xstrdup(new_parts.name);
	if (!fsnode_rename_index(parts.parent, src_index,
							 new_parts.parent, new_name))
		goto fail;
	xfree(strip_oldpath);
	xfree(strip_newpath);
	return 0;
fail:
	xfree(strip_oldpath);
	xfree(strip_newpath);
	return -1;
}

/* LOOPING SUBROUTINE (strlen, vfs_dnode_basename, strcmp,
   fsnode_find_name, fsnode_unlink_index, fsnode_destroy) */
int vfs_unlink(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname)
{
	VFSDnodeName parts;
	unsigned index;
	FSNode *file;
	if (pathname == NULL)
		return -1;

	{ /* Do not allow trailing slashes since you must use `rmdir' to
		 remove a directory.  */
		unsigned path_len = strlen(pathname);
		if (path_len > 0 && pathname[path_len-1] == '/')
			return -1;
	}

	parts = vfs_dnode_basename(vfs_root, vfs_cwd, pathname);
	if (parts.parent == NULL)
		return -1;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		return -1;
	index = fsnode_find_name(parts.parent, parts.name);
	if (index == (unsigned)-1)
		return -1;
	file = parts.parent->data.files.d[index];
	if (file == NULL || (file->delta_mode & DM_ISDIR))
		return -1;
	if (!fsnode_unlink_index(parts.parent, index))
		return -1;
	fsnode_destroy(file);
	return 0;
}

/* Remember, for directories, we always remove trailing slashes from
   the pathname.  */
int vfs_mkdir(FSNode *vfs_root, FSNode *vfs_cwd,
			  const char *pathname, DeltaMode delta_mode)
{
	int retval;
	char *strip_path;
	if (pathname == NULL)
		return -1;
	strip_path = xstrdup(pathname);
	strip_trail_slash(strip_path);
	delta_mode |= DM_ISDIR;
	retval = vfs_ucreat(vfs_root, vfs_cwd, strip_path, delta_mode);
	xfree(strip_path);
	return retval;
}

/* LOOPING SUBROUTINE (xstrdup, strip_trail_slash, fsnode_mkdirp,
   xfree) */
/* Create a directory and any missing path segments.  On failure, the
   newly created directories for the earlier segments are not
   removed.

   Interestingly, this is designed to create intermediate directories
   referenced in a relative path such as "one/two/../three".

   Also, note that this function doesn't check that the final
   mentioned path segment is actually a directory.  It could just as
   well be an existing file and that would be okay.  */
int vfs_mkdirp(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *pathname, DeltaMode delta_mode)
{
	FSNode *parent, *result;
	char *strip_path;
	if (pathname == NULL)
		return -1;
	strip_path = xstrdup(pathname);
	strip_trail_slash(strip_path);
	delta_mode |= DM_ISDIR;
	/* DUPLICATE CODE */
	if (strip_path[0] == '/')
		parent = vfs_root;
	else
		parent = vfs_cwd;
	result = fsnode_mkdirp(parent, strip_path, delta_mode);
	xfree(strip_path);
	if (result == NULL || (result->delta_mode & DM_ISDIR) == 0)
		return 1;
	return 0;
}

/* LOOPING SUBROUTINE (xstrdup, strip_trail_slash, vfs_dnode_basename,
   strcmp, fsnode_find_name, fsnode_unlink_index, fsnode_destroy,
   xfree) */
/* Remember, for directories, we always remove trailing slashes from
   the pathname.  */
int vfs_rmdir(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname)
{
	char *strip_path;
	if (pathname == NULL)
		return -1;
	strip_path = xstrdup(pathname);
	VFSDnodeName parts;
	unsigned index;
	FSNode *dnode;
	strip_trail_slash(strip_path);
	parts = vfs_dnode_basename(vfs_root, vfs_cwd, strip_path);
	if (parts.parent == NULL)
		goto fail;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		goto fail;
	index = fsnode_find_name(parts.parent, parts.name);
	if (index == (unsigned)-1)
		goto fail;
	dnode = parts.parent->data.files.d[index];
	if (dnode == NULL || !(dnode->delta_mode & DM_ISDIR))
		goto fail;
	if (dnode->data.files.len != 0)
		goto fail;
	/* Unix allows you to delete a directory that is still being used
	   as the working directory, but we don't.  */
	if (vfs_cwd == dnode)
		goto fail;
	if (!fsnode_unlink_index(parts.parent, index))
		goto fail;
	fsnode_destroy(dnode);
	xfree(strip_path);
	return 0;
fail:
	xfree(strip_path);
	return -1;
}

/* LOOPING SUBROUTINE (xstrdup, strip_trail_slash, vfs_dnode_basename,
   strcmp, fsnode_find_name, fsnode_unlink_index, fsnode_destroy,
   xfree) */
/* Perform an `rm -r'.  Remember, for directories, we always remove
   trailing slashes from the pathname.  */
int vfs_rmr(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname)
{
	char *strip_path;
	if (pathname == NULL)
		return -1;
	strip_path = xstrdup(pathname);
	VFSDnodeName parts;
	unsigned index;
	FSNode *file;
	strip_trail_slash(strip_path);
	parts = vfs_dnode_basename(vfs_root, vfs_cwd, strip_path);
	if (parts.parent == NULL)
		goto fail;
	/* Do not allow usage of the Unix reserved names "." and "..".  */
	if (!strcmp(parts.name, ".") || !strcmp(parts.name, ".."))
		goto fail;
	index = fsnode_find_name(parts.parent, parts.name);
	if (index == (unsigned)-1)
		goto fail;
	file = parts.parent->data.files.d[index];
	if (file == NULL)
		goto fail;
	/* Unix allows you to delete a directory that is still being used
	   as the working directory, but we don't.  */
	if (vfs_cwd == file)
		goto fail;
	if (!fsnode_unlink_index(parts.parent, index))
		goto fail;
	fsnode_destroy(file);
	xfree(strip_path);
	return 0;
fail:
	xfree(strip_path);
	return -1;
}

/* LOOPING SUBROUTINE (strlen, malloc, strncpy) */
/* LOOPING DIRECT */
char *vfs_getcwd(FSNode *vfs_root, FSNode *vfs_cwd,
				 char *buf, unsigned size)
{
	FSNode *file = vfs_cwd;
	unsigned path_len = 0;
	/* First compute the path length.  */
	if (file != NULL)
	{
		/* Silly Unix.  We need to program a special case for using a
		   "/" for the root directory.  Okay, fine, MS-DOS shares this
		   convention too.  */
		if (file->parent == NULL)
			path_len = 1;
		else
			while (file->parent != NULL)
			{
				path_len += 1 + strlen(file->name);
				file = file->parent;
			}
	}
	/* If the buffer was pre-allocated, check if it is large
	   enough.  */
	if (buf != NULL && path_len + 1 > size)
		return NULL;
	if (buf == NULL)
		buf = (char*)malloc(sizeof(char) * (path_len + 1));
	if (buf == NULL)
		return NULL;
	/* Now copy in the path segments.  */
	file = vfs_cwd;
	buf[path_len] = '\0';
	if (file != NULL)
	{
		/* Silly Unix again.  */
		if (file->parent == NULL)
			buf[0] = '/';
		else
			while (file->parent != NULL)
			{
				unsigned segment_len = strlen(file->name);
				path_len -= segment_len;
				strncpy(&buf[path_len], file->name, segment_len);
				buf[--path_len] = '/';
				file = file->parent;
			}
	}
	return buf;
}

/********************************************************************/

struct vfs_dirent s_vfs_dirent;

/* LOOPING SUBROUTINE (malloc) */
VFSDir *vfs_fopendir(FSNode *dnode)
{
	VFSDir *vfs_dir;
	if (dnode == NULL || !(dnode->delta_mode & DM_ISDIR))
		return NULL;
	vfs_dir = (VFSDir*)malloc(sizeof(VFSDir));
	if (vfs_dir == NULL)
		return NULL;
	vfs_dir->dnode = dnode;
	vfs_dir->i = 0;
	return vfs_dir;
}

/* LOOPING SUBROUTINE (vfs_fopendir, vfs_traverse) */
VFSDir *vfs_opendir(FSNode *vfs_root, FSNode *vfs_cwd, const char *name)
{
	return vfs_fopendir(vfs_traverse(vfs_root, vfs_cwd, name, 0));
}

/* LOOPING SUBROUTINE (strlen) */
/* NOTE: A POSIX conformant implementation would return "." and ".."
   directory entries.  However, for our purposes, we don't return
   these since we consider them merely path semantics rather than
   actual directory entries.  (Unless, of course, there are entries
   with the names "." and "..".)

   Also, be forewarned that modifying the directory as you are
   traversing it will give you incorrect results.  */
struct vfs_dirent *vfs_readdir(VFSDir *vfs_dir)
{
	FSNode *dnode;
	if (vfs_dir == NULL || vfs_dir->i >= vfs_dir->dnode->data.files.len)
		return NULL;
	dnode = vfs_dir->dnode->data.files.d[vfs_dir->i++];
	s_vfs_dirent.d_ino = 0;
	s_vfs_dirent.d_name = dnode->name;
	s_vfs_dirent.d_namlen = strlen(dnode->name);
	return &s_vfs_dirent;
}

void vfs_rewinddir(VFSDir *vfs_dir)
{
	if (vfs_dir == NULL)
		return;
	vfs_dir->i = 0;
}

void vfs_seekdir(VFSDir *vfs_dir, long offset)
{
	unsigned num_files;
	if (vfs_dir == NULL)
		return;
	num_files = vfs_dir->dnode->data.files.len;
	if ((unsigned)offset <= num_files)
		vfs_dir->i = offset;
	else
		vfs_dir->i = num_files;
}

long vfs_telldir(VFSDir *vfs_dir)
{
	if (vfs_dir == NULL)
		return -1;
	return vfs_dir->i;
}

/* LOOPING SUBROUTINE (xfree) */
int vfs_closedir(VFSDir *vfs_dir)
{
	xfree(vfs_dir);
	return 0;
}
