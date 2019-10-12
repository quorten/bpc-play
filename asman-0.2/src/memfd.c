/* memfd.c -- A simple implementation of an in-memory file.

Copyright (C) 2019 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/*

Required methods:

memfd_create()
memfd_destroy()
memfd_ftruncate()
memfd_read()
memfd_write()

Important note!  This is strictly a model of in-memory files,
INDEPENDENT OF an in-memory filesystem.

Therefore, the following functions are not applicable:

unixfd_open()
unixfd_close()
unixfd_lseek()
unixfd_fsync()
unixfd_fdatasync()

unixfd_pipe()
unixfd_dup()
unixfd_dup2()
unixfd_dup3()
unixfd_fcntl()

unixfd_openpty()
// unixfd_forkpty()
unixfd_login_tty()
unixfd_termios()
unixfd_ioctl()
unixfd_ttyname()
unixfd_isatty()

Instead, they are implemented as another higher-level layer, `unixfd'.

The method `unixfd_lseek()' is implemented only at the higher level,
i.e. after a user issues an "open" to create their own file
descriptor.

An in-memory file is a list of block pointers of the designated file
block size.  Currently, there is only a single global context variable
for file block size, and its default value is 4096.

At first thought for a computer moderner, this simple-minded data
structure surely doesn't work very efficiently for large files, and
although that is true, a few back-of-the-hand calculations can show
that with minor tweaks, these methods work reasonably well for a
computer with at least 8 MB of RAM and maximum hard drive capacity of
8 GB.

Indeed, the FAT32 filesystem used in Windows 98 has served us well!

For hard drives larger than 8 GB, sure, you do need a more
sophisticated file block handling methodology.

Maximum file size is 4 GB.

TODO: An extension of my own convenience.  Add functions for reading
files backwards.  Of course this doesn't work on stream files, only
seekable disk files.

*/

#include "bool.h"
#include "exparray.h"
#include "memfd.h"

/* Must be a power of two so that `g_memfd_blksize_bits' can be used
   to efficiently multiply and divide by shifting.  */
unsigned g_memfd_blksize = 4096;
unsigned g_memfd_blkmask = g_memfd_blksize - 1;
unsigned g_memfd_blksize_bits = 12;

typedef unsigned char* MemFDBlk_ptr;
EA_TYPE(MemFDBlk);

struct MemFD_tag
{
	MemFDBlk_ptr_array blocks;
	unsigned file_size;
};
typedef struct MemFD_tag MemFD;

MemFD *memfd_create(void)
{
	MemFD *file = (MemFD*)malloc(sizeof(MemFD));
	if (file == NULL)
		return NULL;
	EA_INIT(file->blocks, 16);
	file->file_size = 0;
	return file;
}

void memfd_destroy(MemFD *file)
{
	MemFDBlk_ptr *blocks_d = file->blocks.d;
	unsigned blocks_len = file->blocks.len;
	unsigned i;
	for (i = 0; i < blocks_len; i++)
		free(blocks_d[i]);
	EA_DESTROY(file->blocks);
	free(file);
}

int memfd_ftruncate(MemFD *file, unsigned length)
{
	bool alloc_error = false;

	/* Compute the new number of blocks.  */
	unsigned new_num_blks = length >> g_memfd_blksize_bits;

	/* Free blocks if necessary.  */
	if (new_num_blks < file->blocks.len)
	{
		MemFDBlk_ptr *blocks_d = file->blocks.d;
		unsigned old_num_blks = file->blocks.len;
		unsigned i;
		for (i = new_num_blks; i < old_num_blks; i++)
			free(blocks_d[i]);
		file->blocks.len = new_num_blks;
		EA_NORMALIZE(file->blocks);
	}
	/* Allocate blocks if necessary.  */
	else if (new_num_blks > file->blocks.len)
	{
		MemFDBlk_ptr *blocks_d;
		unsigned old_num_blks = file->blocks.len;
		EA_SET_SIZE(file->blocks, new_num_blks);
		blocks_d = file->blocks.d;
		for (i = old_num_blks; i < new_num_blks; i++)
		{
			MemFDBlk *new_blk = (MemFDBlk*)malloc(g_memfd_blksize);
			if (new_blk == NULL)
			{
				alloc_error = true;
				break;
			}
			else
				memset(new_blk, 0, g_memfd_blksize);
			blocks_d[i] = new_blk;
		}
		if (alloc_error)
			EA_SET_SIZE(file->blocks, old_num_blks);
	}
	else /* (new_num_blks == file->blocks.len) */
	{
		/* Number of blocks is equal, but we may need to zero new new
		   bytes added to the file.  */
		if (length > file->file_size)
		{
			unsigned old_blk_ofs = (file->file_size) & g_memfd_blkmask;
			unsigned new_blk_ofs = length & g_memfd_blkmask;
			memset(&file->blocks.d[file->blocks.len-1][old_blk_ofs],
				   0, new_blk_ofs - old_blk_ofs);
		}
	}

	if (alloc_error)
		return -1;
	file->file_size = length;
	return 0;
}

#define MEMFD_READ 0
#define MEMFD_WRITE 1

int memfd_blkcopy(unsigned way, MemFD *file,
				  unsigned offset, void *buffer, unsigned count)
{
	unsigned char *buf_ptr = buffer;
	unsigned blk_idx;
	MemFDBlk_ptr *blocks_d = file->blocks.d;

	unsigned start_blk_num = offset >> g_memfd_blksize_bits;
	unsigned start_blk_ofs = offset & g_memfd_blkmask;
	unsigned start_blk_size;
	unsigned end_addr = offset + count;
	unsigned end_blk_num = end_addr >> g_memfd_blksize_bits;
	unsigned end_blk_ofs = end_addr & g_memfd_blkmask;

	{
		unsigned blocks_len = file->blocks.len;
		if (way == MEMFD_READ)
		{
			/* TODO FIXME: Read up until the end of the file if
			   possible.  Only if nothing can be read should we return
			   -1.  */
			if (end_blk_num > blocks_len ||
				(end_blk_num == blocks_len && end_blk_ofs != 0))
				return -1;
		}
		else /* (way == MEMFD_WRITE) */
		{
			/* TODO FIXME */
		}
	}

	if (start_blk_num == end_blk_num)
		start_blk_size = end_blk_ofs - start_blk_ofs;
	else
		start_blk_size = g_memfd_blksize - start_blk_ofs;

	/* Copy the first partial block.  */
	blk_idx = start_blk_num;
	if (way == MEMFD_READ)
		memcpy(buf_ptr, &blocks_d[blk_idx][start_blk_ofs], start_blk_size);
	else /* (way == MEMFD_WRITE) */
		memcpy(&blocks_d[blk_idx][start_blk_ofs], buf_ptr, start_blk_size);
	buf_ptr += start_blk_size; blk_idx++;

	/* Copy the full blocks, if applicable.  */
	while (blk_idx < end_blk_num)
	{
		if (way == MEMFD_READ)
			memcpy(buf_ptr, blocks_d[blk_idx], g_memfd_blksize);
		else /* (way == MEMFD_WRITE) */
			memcpy(blocks_d[blk_idx], buf_ptr, g_memfd_blksize);
		buf_ptr += g_memfd_blksize; blk_idx++;
	}

	/* Copy the last partial block, if applicable.  */
	if (end_blk_num != start_blk_num && end_blk_ofs > 0)
	{
		if (way == MEMFD_READ)
			memcpy(buf_ptr, blocks_d[blk_idx], end_blk_ofs);
		else /* (way == MEMFD_WRITE) */
			memcpy(blocks_d[blk_idx], buf_ptr, end_blk_ofs);
		buf_ptr += end_blk_ofs;
	}

	return count;
}

int memfd_read(MemFD *file, unsigned offset, void *buffer, unsigned count)
{
	return memfd_blkcopy(MEMFD_READ, file, offset, buffer, count);
}

int memfd_write(MemFD *file, unsigned offset, void *buffer, unsigned count)
{
	return memfd_blkcopy(MEMFD_WRITE, file, offset, buffer, count);
}

/********************************************************************/
/* Now we also have a really dumb in-memory file implementation.
   Whole file is basically JUST an exparray, but with 64-bit file
   size.  This is more efficient for tiny files than a block-based
   methodology.  */

/* TODO FIXME: Ensure that 64-bit size is being used.  Mainly so this
   is plug-compatible with my VFS code, not that you'd ever want to
   use such a simple-minded filesystem for such large files.  */

DumFD *dumfd_create(void)
{
	DumFD *file = (DumFD*)malloc(sizeof(DumFD));
	if (file == NULL)
		return NULL;
	EA_INIT(*file, 16);
	return file;
}

void dumfd_destroy(DumFD *file)
{
	EA_DESTROY(*file);
	free(file);
}

int dumfd_ftruncate(DumFD *file, unsigned length)
{
	unsigned old_size = file->len;
	EA_SET_SIZE(*file, length);
	if (file->d == NULL)
		return -1;
	if (length > old_size)
		memset(&file->d[old_size], 0, length - old_size);
	return 0;
}

int dumfd_read(DumFD *file, unsigned offset, void *buffer, unsigned count)
{
	/* TODO FIXME: Read up until the end of the file if possible.
	   Only if nothing can be read should we return -1.  */
	if (offset + count > file->len)
		return -1;
	memcpy(buffer, &file->d[offset], count);
	return count;
}

int dumfd_write(DumFD *file, unsigned offset, void *buffer, unsigned count)
{
	if (offset + count > file->len)
		EA_SET_SIZE(*file, offset + count);
	memcpy(&file->d[offset], buffer, count);
	return count;
}
