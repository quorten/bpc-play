/* Copyright 2018, see COPYING.  */

/* Block modification map: Track changes to a dirty block map given
   Unix-like write operations.  A map can then be used for efficient
   delta syncs.  */

/* NOTE: A related system.  Track reads for the purposes of caching.
   Of course this is not necessary for data synchronization, as reads
   change nothing.  Matter of fact, this exact same system can be used
   for precisely that purpose.  */

/* NOTE: Another related application.  Software MMU.  Unix-style reads
   and writes are used to obtain blocks of data at a time.  If the
   software is trusted or if higher-level operations are not needed,
   instead of copying, direct pointers to the blocks in memory can be
   passed.  Curiously, EINTR can be used for a seamless compatible
   protocol for segmented blocks.

   Finally, it is also possible for a compiler to wrap all memory
   accesses into the software MMU protocol.  Notably, in a system that
   does not require block copying, the overhead is limited by the
   block size and pointer caching.

   Oh, and one more thing.  Block locks/unlocks.  That's a toughie for
   pre-emptive multitasking.  The idea is to keep the locked block
   segments small enough so that the process plays well with the time
   slices available under pre-emptive multitasking.  Otherwise you
   have to raise a signal to the process that has its blocks locked
   that you want to move.  */

/* Another application: The map can be used for copy-on-write
   (COW).  */

/* TODO MOVE: Nice to have features for a high-level filesystem: Split
   one file into two, join two files together, copy-on-write
   clones.  Union mounts, whiteouts.  */

#include <unistd.h>
#include <string.h>

#include "xmalloc.h"

typedef struct BlockMod_tag BlockMod;
struct BlockMod_tag
{
	/* Sizes are measured in bytes.  */
	unsigned length;
	/* 2 ^ (blkbits) = blksize */
	unsigned blkbits;
	unsigned blksize;
	/* Bitmap of blocks, zero for unmodified, one for modified.  Least
	   significant bit = block 0.  */
	unsigned char *blkmap;
	/* Current position pointer, used to support Unix-like writing.  */
	unsigned offset;
};

/* Return the base 2 logarithm of a positive power-of-two number.
   Returns (unsigned)-1 on error.  */
unsigned
log2(unsigned n)
{
	unsigned exp = 0;
	if (n == 0 || (n & (n - 1)) != 0)
		return (unsigned)-1;
	while (n)
	{
		n >>= 1;
		exp++;
	}
	return exp;
}

/* Change the size of the block map.  */
int bmodmap_ftruncate(BlockMod *mod_map, unsigned length)
{
	unsigned blksize = mod_map->blksize;
	unsigned old_num_blocks =
		mod_map->length >> mod_map->blkbits +
		((length & (blksize - 1)) ? 1 : 0);
	unsigned old_num_bit_blocks =
		mod_map->num_blocks >> 3 + ((num_blocks & (8 - 1)) ? 1 : 0);
	unsigned num_blocks =
		length >> mod_map->blkbits + ((length & (blksize - 1)) ? 1 : 0);
	unsigned num_bit_blocks =
		num_blocks >> 3 + ((num_blocks & (8 - 1)) ? 1 : 0);
	unsigned num_new_bit_blocks = num_bit_blocks - old_num_bit_blocks;
	mod_map->length = length;
	mod_map->blkmap = (unsigned char*)
		xrealloc(mod_map->blkmap, sizeof(unsigned char) * num_bit_blocks);
	/* Zero the new block map entries.  */
	memset(mod_map->blkmap + old_num_bit_blocks, 0, num_new_bit_blocks);
}

/* Address and size measured in bytes.  */
void bmodmap_lseek_write(BlockMod *mod_map, unsigned offset, unsigned size)
{
	unsigned end = offset + size;
	unsigned blksize = mod_map->blksize;
	unsigned begin_block = offset & ~(blksize - 1);
	unsigned end_block = end & ~(blksize - 1);
	unsigned char map_bits_begin = ~0 << (begin_block & (8 - 1));
	unsigned char map_bits_end = ~(~0 << (end_block & (8 - 1)));
	unsigned map_byte_begin = begin_block >> 3;
	unsigned map_byte_end = end_block >> 3;
	bool write_end_bits = (map_bits_end & (8 - 1)) ? true : false;
	unsigned i;

	/* If necessary, extend the block map to accommodate writes.  */
	if (end > mod_map->length)
		bmodmap_ftruncate(mod_map, end);

	mod_map->blkmap[map_byte_begin] |= map_bits_begin;
	for (i = map_byte_begin + 1; i < map_byte_end; i++)
		mod_map->blkmap[i] |= ~0;
	if (write_end_bits)
		mod_map->blkmap[i] |= map_bits_end;
}

/* Unix-style write operation.  */
int bmodmap_write(BlockMod *mod_map, unsigned size)
{
	bmodmap_lseek_write(mod_map, mod_map->offset, size);
	mod_map->offset += size;
	return size;
}

/* # define SEEK_SET	0	/\* Seek from beginning of file.  *\/ */
/* # define SEEK_CUR	1	/\* Seek from current position.  *\/ */
/* # define SEEK_END	2	/\* Seek from end of file.  *\/ */

/* Unix-style lseek operation.  */
int lseek(BlockMod *mod_map, int offset, int whence)
{
	switch (whence)
	{
	case SEEK_SET:
		mod_map->offset = (unsigned)offset;
		break;
	case SEEK_CUR:
		mod_map->offset += offset;
		break;
	case SEEK_END:
		mod_map->offset = mod_map->length + offset;
		break;
	default:
		return -1;
	}
	return (int)mod_map->offset;
}

/* Get the block map for use.  Currently this simply returns the
   context structure itself.  */
BlockMod* bmodmap_get_map(BlockMod *mod_map)
{
	return mod_map;
}

/* Clear the block map after a sync has been completed.  */
void bmodmap_fsync(BlockMod *mod_map)
{
	unsigned blksize = mod_map->blksize;
	unsigned num_blocks =
		length >> mod_map->blkbits + ((length & (blksize - 1)) ? 1 : 0);
	unsigned num_bit_blocks =
		num_blocks >> 3 + ((num_blocks & (8 - 1)) ? 1 : 0);
	memset(mod_map->blkmap, 0, sizeof(unsigned char) * num_bit_blocks);
}

/* Initialize a new block map.  */
void bmodmap_init(BlockMod *mod_map)
{
	mod_map->length = 0;
	mod_map->blkmap = NULL;
	mod_map->offset = 0;
}

/* Free the memory associated with a block map.  */
void bmodmap_destroy(BlockMod *mod_map)
{
	xfree(mod_map->blkmap = NULL);
}
