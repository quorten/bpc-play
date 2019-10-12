/* vfs.h -- Model an in-memory file system.

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

/* The purpose of implementing an in-memory file system is to support
   efficient computations on file system data.  The previous
   implementation was entirely based off of sorted strings of full
   path names, and therefore it wasn't very efficient, even though it
   worked.  */

#ifndef VFS_H
#define VFS_H

#include "bool.h"
#include "exparray.h"
/* #include "strheap.h" */

/* We have to be careful not to use the exact same names as the
   standard C library in our data structures because macros can garble
   up our code if it matches.  */

typedef unsigned FSint32;
typedef unsigned long long FSint64;
typedef FSint32 FStime32;
typedef FSint32 FSuid;
typedef FSint32 FSgid;

#define DNODE_EXT \
	char *name;

#define DNODE_PARENT_EXT(NodeType) \
	NodeType *parent;

#define DNODE_DATA(NodeType_array) \
	NodeType_array files;

/* The most basic file system features from Disk Operating Systems
   (DOS).  In particular, the popular MS-DOS and its File Allocation
   Table (FAT) are inspirations.  Note that the /very/ very earliest
   versions of 86-DOS and PC DOS did not support file modification
   times.  Also, note that the interpretation and number of mode flags
   varies wildly across operating systems.  */

#define QDOSINODE_EXT(SizeType) \
	SizeType st__mode; \
	SizeType st__size;

#define QDOSINODE_D_EXT \
	unsigned char *data;

#define DOSINODE_EXT(TimeType) \
	TimeType st__mtime;

typedef struct DosINode_tag DosINode;

struct DosINode_tag
{
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
};

typedef struct DosNode_tag DosNode;

struct DosNode_tag
{
	DNODE_EXT;
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
};

/* The extended feature of "resource forks" was most famously used in
   the Macintosh's Macintosh File System (MFS) and Hierarchical File
   System (HFS).  The Macintosh file system was also one of the early
   systems to introduce the file creation timestamp.  */

#define CRTIME_EXT(TimeType) \
	TimeType st__crtime;

#define MACINODE_EXT(SizeType) \
	SizeType rsrc_size;

#define MACINODE_D_EXT \
	unsigned char *rsrc;

typedef struct MacINode_tag MacINode;

struct MacINode_tag
{
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	CRTIME_EXT(FStime32);
	MACINODE_EXT(FSint32);
};

typedef struct MacNode_tag MacNode;

struct MacNode_tag
{
	DNODE_EXT;
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	MACINODE_EXT(FSint32);
};

/* Versions of Microsoft Windows such as Microsoft Windows 95 were
   popular for introducing a file creation timestamp ("birthtime" in
   BSD vocabulary), and a file access time.  Long File Names (LFN) was
   another popular introduction, but note that the LFN API can be
   utilized on DOS too.  */

#define ATIME_EXT(TimeType) \
	TimeType st__atime;

typedef struct WinINode_tag WinINode;

struct WinINode_tag
{
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	CRTIME_EXT(FStime32);
};

typedef struct WinNode_tag WinNode;

struct WinNode_tag
{
	DNODE_EXT;
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	CRTIME_EXT(FStime32);
};

/* The key advancements of a Unix file system over a Windows 95 file
   system are those of permissions, a "ctime" timestamp, and "inodes."
   The "ctime" timestamp is updated whenever the file's attributes or
   data is updated.  "inodes" essentially allow "hardlinks" such that
   a single file can symmetrically exist in multiple directories.

   The key disadvantage of a Unix file system over a Windows 95 file
   system is the lack of a creation timestamp.  */

#define UNIXINODE_EXT(SizeType, TimeType) \
	FSuid uid; \
	FSgid gid; \
	TimeType st__ctime; \
	SizeType st__ino; \
	unsigned st__nlink;

typedef struct UnixINode_tag UnixINode;

struct UnixINode_tag
{
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	UNIXINODE_EXT(FSint32, FStime32);
};

typedef struct DNode_tag DNode;

struct DNode_tag
{
	DNODE_EXT;
	UnixINode *inode;
};

/* The key advancement of a 64-bit Unix file system is support for
   larger files and more files through the use of 64-bit integers.
   The timestamp range and granularity is also increased.  For all
   practical concerns, 64 bit file systems that do not have enough
   features to be considered a "Unix" file system are not worth
   modeling.  */

#define UNIXINODE64_EXT(TimeExtType) \
	TimeExtType st__mtime_usec; \
	TimeExtType st__ctime_usec; \
	TimeExtType st__atime_usec;

typedef struct UnixINode64_tag UnixINode64;

struct UnixINode64_tag
{
	QDOSINODE_EXT(FSint64);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	UNIXINODE_EXT(FSint64, FStime32);
	UNIXINODE64_EXT(FSint32);
};

typedef struct DNode64_tag DNode64;

struct DNode64_tag
{
	DNODE_EXT;
	UnixINode64 *inode;
};

/* The key advancement of a Windows NT file system over a standard
   Unix filesystem is the addition of creation timestamps.  */

/* Windows NT file systems also support Alternate Data Streams (ADS),
   more commonly known as named Forks, and XAttrs.  NTFS actually
   implements both as "attributes," and it also has "security
   descriptors."  Yes, in other words, NTFS has all the features, even
   those that are seldom used.  */

/* So, these are just "basic" Windows NT data structures, "BNT".
   Technically, Windows NT file systems are only available in 64-bit,
   but we include 32-bit data structures here for application
   implementation flexibility.  */

typedef struct BNTINode32_tag BNTINode32;

struct BNTINode32_tag
{
	QDOSINODE_EXT(FSint32);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	CRTIME_EXT(FStime32);
	UNIXINODE_EXT(FSint32, FStime32);
};

typedef struct BNTDNode32_tag BNTDNode32;

struct BNTDNode32_tag
{
	DNODE_EXT;
	BNTINode32 *inode;
};

#define BNTINODE_EXT(TimeExtType) \
	TimeExtType st__crtime_usec;

typedef struct BNTINode_tag BNTINode;

struct BNTINode_tag
{
	QDOSINODE_EXT(FSint64);
	DOSINODE_EXT(FStime32);
	ATIME_EXT(FStime32);
	CRTIME_EXT(FStime32);
	UNIXINODE_EXT(FSint64, FStime32);
	UNIXINODE64_EXT(FSint32);
	BNTINODE_EXT(FSint32);
};

typedef struct BNTDNode_tag BNTDNode;

struct BNTDNode_tag
{
	DNODE_EXT;
	BNTINode *inode;
};

/* XAttrs and named Forks?  Those are not supported in our basic `vfs'
   implementation because they are esoteric and seldom used.  See
   `avfs.h' for an implementation.

   The primary practical use for XAttrs is advanced security
   attributes.  SELinux is a particular example.  It is perhaps a
   better idea for applications that do need those attributes to
   implement them directly in their internal data structures rather
   than as an XAttr.  */

/********************************************************************/

/* It turns out that our file system data structures for the
   simplifier only model a QDOS file system.  Pretty stupid, except
   that we support long file names and nested directories.  */

typedef struct FileIdent_tag FileIdent;

struct FileIdent_tag
{
	FSint32 st__size;
	/* unsigned char cksum[4];
	unsigned char md5sum[16];
	unsigned char sha256sum[32]; */
};

typedef enum DeltaMode_tag DeltaMode;

enum DeltaMode_tag
{
	DM_SRC = 0, /* node definitely exists as a (source) file */

	/* Source file modes */
	DM_DNE = 1, /* node marks that a filename does not exist */
	DM_ICR = 2, /* node marks "implicit create": if a source file
				   doesn't exist, create it */
	DM_IRM = 3, /* node marks "implicit delete": if a source file
				   exists, delete it, but ignore failures when
				   attempting to delete non-existent source file */

	/* Destination file modes */
	DM_NEW = 4, /* node indicates created file/directory */
	DM_UPDATE = 5, /* node indicates existing file that is updated */
	DM_TOUCH = 6, /* node is either an updated or created file */

	/* These modes aren't actually supported.  */
	DM_COPY = 7, /* destination is copied from source file */
	DM_LINK = 8, /* destination is hard linked from link source */

	/* OR'ed flags */
	DM_ISDIR = 16,
	DM_ISLNK = 32,
	DM_WHITEOUT = 64, /* a file was deleted at this node's name
					     ("whiteout")  */
};

typedef struct FSNode_tag FSNode;
typedef FSNode* FSNode_ptr;
EA_TYPE(FSNode);
EA_TYPE(FSNode_ptr);

struct FSNode_tag
{
	union
	{
		/* For regular files only: */
		/* FileIdent ident; */
		/* struct gdata_tag {
			unsigned char *d;
			FSint32 len;
		} data; */
		struct gdata64_tag {
			unsigned char *d;
			FSint64 len;
		} data64;
		/* For directories only: */
		FSNode_ptr_array files;
		/* For symlinks only: */
		/* StrNode *target; */
	} data;

	DeltaMode delta_mode;
	/* StrNode */ char *name;
	FSNode *parent;
	/* In the source node, points to the destination node.  In the
	   destination node, points to the source node.  NULL is used for
	   deleted and created files.  */
	FSNode *other;

	/* More attributes, mostly for fun, not a mandatory requirement
	   for all of the application tools.  */
	FSint64 st__mtime;
	/* TimeType st__mtime;
	TimeType st__crtime;
	FSuid uid;
	FSgid gid;

	SizeType st__ino;

	unsigned st__nlink; */

	/* Hard linked files/directories have multiple parents.  */
	/* FSNode_ptr_array parents; */
	/* If a file gets copied or hard linked, then subsequently
	   deleted, we need to keep track of the references to this file
	   so that when we output the delta commands, we can hard link to
	   an existing reference.  */
	/* FSNode_ptr_array references; */
};

/********************************************************************/
/* Low-level, file system node functions.  */

extern bool g_vfs_sorted;
extern bool g_vfs_fast_insert;

FSNode *fsnode_create(char *name, DeltaMode delta_mode);
void fsnode_destroy(FSNode *node);
unsigned fsnode_find_inode(FSNode *parent, FSNode *file);
unsigned fsnode_find_name(FSNode *parent, const char *name);
bool fsnode_sort_names(FSNode *node);
bool fsnode_link(FSNode *parent, FSNode *file);
bool fsnode_unlink_index(FSNode *parent, unsigned index);
FSNode *fsnode_ucreat(FSNode *parent, char *name, DeltaMode delta_mode);
bool fsnode_rename_index(FSNode *parent, unsigned index,
						 FSNode *new_parent, char *new_name);
FSNode *fsnode_traverse(FSNode *parent, char *path);
FSNode *fsnode_mkdirp(FSNode *parent, char *path, DeltaMode delta_mode);

/********************************************************************/
/* High-level VFS functions, similar to Unix system calls.  */

typedef struct VFSDnodeName_tag VFSDnodeName;

struct VFSDnodeName_tag
{
	FSNode *parent;
	const char *name;
};

FSNode *vfs_traverse(FSNode *vfs_root, FSNode *vfs_cwd,
					 const char *path, unsigned len);
const char *vfs_basename(const char *pathname);
VFSDnodeName vfs_dnode_basename(FSNode *vfs_root, FSNode *vfs_cwd,
								const char *pathname);
unsigned strip_trail_slash(char *path);
FSNode *vfs_create(void);
void vfs_destroy(FSNode *vfs_root);
FSNode *vfs_stat(FSNode *vfs_root, FSNode *vfs_cwd, const char *path);
FSNode *vfs_fchdir(FSNode *dnode);
FSNode *vfs_chdir(FSNode *vfs_root, FSNode *vfs_cwd, const char *path);
int vfs_ucreat(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *pathname, DeltaMode delta_mode);
int vfs_ftruncate(FSNode *file, FSint64 length);
int vfs_truncate(FSNode *vfs_root, FSNode *vfs_cwd,
				 const char *pathname, FSint64 length);
int vfs_futime(FSNode *file, FSint64 *times);
int vfs_utime(FSNode *vfs_root, FSNode *vfs_cwd,
			  const char *pathname, FSint64 *times);
int vfs_rename(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *oldpath, const char *newpath);
int vfs_unlink(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname);
int vfs_mkdir(FSNode *vfs_root, FSNode *vfs_cwd,
			  const char *pathname, DeltaMode delta_mode);
int vfs_mkdirp(FSNode *vfs_root, FSNode *vfs_cwd,
			   const char *pathname, DeltaMode delta_mode);
int vfs_rmdir(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname);
int vfs_rmr(FSNode *vfs_root, FSNode *vfs_cwd, const char *pathname);
char *vfs_getcwd(FSNode *vfs_root, FSNode *vfs_cwd,
				 char *buf, unsigned size);

/********************************************************************/
/* `dirent', VFS-edition.  */

typedef struct VFSDir_tag VFSDir;

struct VFSDir_tag
{
	FSNode *dnode;
	unsigned i;
};

struct vfs_dirent
{
	FSint32 d_ino;
	char *d_name;
	unsigned d_namlen;
};

VFSDir *vfs_fopendir(FSNode *dnode);
VFSDir *vfs_opendir(FSNode *vfs_root, FSNode *vfs_cwd, const char *name);
struct vfs_dirent *vfs_readdir(VFSDir *vfs_dir);
void vfs_rewinddir(VFSDir *vfs_dir);
void vfs_seekdir(VFSDir *vfs_dir, long offset);
long vfs_telldir(VFSDir *vfs_dir);
int vfs_closedir(VFSDir *vfs_dir);

#endif /* not VFS_H */
