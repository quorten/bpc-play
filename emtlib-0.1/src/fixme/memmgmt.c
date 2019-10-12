/* Core memory management functionality.

   This code is mainly designed to support extensible memory
   management.  Usually, when there are large blocks of extensive
   memory, moving the entire blocks can be degrading to performance,
   especially when the memory management is performed synchronously.
   Non-extensive memory is handled very well by the standard C
   library.  Well, maybe that's not completely true.  Small blocks are
   not allocated very efficiently.  Well, if you want control over you
   memory manager, you will allocate blocks of memory that you can use
   with these private heap function.  Memory cannot span across
   multiple private heaps.  If such a situation occurs, then you will
   have to revert back allocating from the system heap to reconstruct
   the private heap.  You should warn the user first, though.  This
   problem can be somewhat mitigated on multithreaded operating
   systems by using multiple heaps.

   So here's my conclusions: you need both a large block memory
   manager and a small block memory manager.  Now you can have
   both.  */

#include <stdlib.h>
#include <math.h>

#include "core.h"

/* Header Declarations */
struct Block_Info_t;
typedef struct Block_Info_t Block_Info;

struct Block_Info_t
{
	unsigned size;
	Block_Info* next_block;
};

/* Memory Manager Specific Variables */
unsigned heap_size;
unsigned num_blocks;
Block_Info** blocks;
unsigned num_free_blks;
unsigned free_blk_one; /* first free block */
bool free_blk_avail; /* Are there still free blocks?  */
/* The variable 'num_handles' corresponds to the number of resident
   initial allocations.  */
unsigned num_handles;
bool max_blks_set;
bool mmg_active = false;

/* Initialize the local memory manager.  The variable 'new_heap_size'
   is a measure of bytes, and it may not be equal to zero.  If
   'max_blocks' is equal to zero, then there is no block limit.

   On success, this function will return 'true'. On failure, it will
   return 'false'.  */
bool mmg_init(unsigned new_heap_size, unsigned max_blocks)
{
	unsigned i;

	heap_size = new_heap_size;
	num_blocks = 0;
	num_handles = 0;

	set_max_blocks(max_blocks);

	blocks = (Block_Info**)malloc(heap_size);
	if (blocks == NULL)
		return false;
	free_blk_avail = true;
	free_blk_one = 0;
	blocks[free_blk_one].size = heap_size - sizeof(Block_Info);
	blocks[free_blk_one].nextBlock = NULL;

	mmg_active = true;

	return true;
}

/* Shutdown the local memory manager.

   On success, this function will return 'true'. On failure, it will
   return 'false'.  */
void mmg_shutdown()
{
	free(blocks);
	mmg_active = false;
}

/* Sets or clears the maximum number of available block slots.  If
   zero is passed as a parameter, then the number of available block
   slots will be set based on an estimate.  If there is a maximum set,
   then the local heap will be automatically defragmented in
   syncronous mode if this limit is reached.  New allocations will be
   denied if it causes the maximum number of blocks to exceed the
   limit.

   On success, this function will return 'true'. On failure, it will
   return 'false'.  */
bool set_max_blocks(unsigned max_blocks)
{
	if (max_blocks == 0)
	{
		/* Estimate a good number of blocks for our heap size.  */
		num_free_blks = (unsigned)((sqrt(2 * heap_size + 0.25) - 0.5)
								   / 2);
		num_free_blks += heap_size % 2;
		max_blks_set = false;
	}
	else
	{
		num_free_blks = max_blocks;
		max_blks_set = true;
	}

	return true;
}

/* Sets the memory manager local heap size in bytes.  This function
   will not automatically expand or shrink the number of available
   block slots.  Block slot expansion will occur automatically at
   allocations, if applicable.  Block slot shrinking will occur only
   on a call to 'set_max_blocks' with a parameter of zero.

   On success, this function will return 'true'. On failure, it will
   return 'false'.  */
bool set_heap_size(unsigned new_heap_size)
{
	void* alloc_test;
	Block_Info* last_free_blk;

	alloc_test = realloc(blocks, new_heap_size);
	if (alloc_test == NULL)
		return false;

	blocks = (Block_Info*)alloc_test;

	/* Find or create the last free block.  */
	if (free_blk_avail == true)
	{
		last_free_blk = free_blk_one;
		while (blocks[last_free_blk].next_block != NULL)
			last_free_blk = blocks[last_free_blk].next_block;
		blocks[last_free_blk].size += new_heap_size - heap_size;
	}
	else
	{
		if (num_free_blks != 0)
		{
			free_blk_one = num_blocks;
			blocks[free_blk_one] = (blocks[num_blocks-1] +
									blocks[num_blocks-1].size);
			blocks[free_blk_one].size = (new_heap_size - heap_size -
										 sizeof(Block_Info));
			blocks[free_blk_one].next_block = NULL;
		}
		else
		{
			set_max_blocks(0);
		}
	}

	heap_size = new_heap_size;

	return true;
}

/* Allocate an initial block of memory.  The variable 'size' is a
   measure of bytes.  If 'use_mmg' is false, then the block of memory
   is never allocated on the local heap.  */
Block_Info* mmg_alloc(unsigned size, bool use_mmg)
{
	if (mmg_active == true && use_mmg == true)
	{
		unsigned i;

		i = free_blk_one;
		while (true)
		{
			/* Check the size of this free block.  */
			if (blocks[i].size >= size)
				break;
			else
			{
				i = blocks[i].next_block;
				if (i == NULL)
					return NULL;
			}
		}
	free_blk_one = 0;
	blocks[free_blk_one].nextBlock = NULL;

		return NULL;
	}
	else
	{
		Block_Info* new_block;
		new_block = malloc(sizeof(Block_Info) + size);
		new_block->size = size;
		new_block->next_block = NULL;
		return new_block;
	}
}

/* Reallocate a block of memory.  If 'segment' is 'true', then a
   second block of memory will be allocated if the memory manager
   cannot expand the block or the memory manager is not active.  */
Block_Info* mmg_realloc(Block_Info* memory, unsigned new_size, bool segment)
{
	if (mmg_active == true)
	{
		return NULL;
	}
	else
	{
		if (segment == true)
		{
			unsigned add_size;
			Block_Info* new_block;

			add_size = new_size - memory->size;
			new_block = (Block_Info*)malloc(add_size);
			if (new_block == NULL)
				return NULL;

			memory->next_block = new_block;
			new_block->size = add_size;
			new_block->next_block = NULL;
			return new_block;
		}
		else
		{
			Block_Info* new_ptr;
			new_ptr = (Block_Info*)realloc((void*)memory, new_size);
			if (new_ptr == NULL)
				return NULL;
			new_ptr->size = new_size;
			return new_ptr;
		}
	}
	return NULL;
}

/* Free a block of memory manager memory.  Do not try to use this on
   memory not allocated with this memory manager.  */
void mmg_free(void* memory)
{
}

/* Defragments a block or blocks allocated in segments.  If
   synchronous is false, then the function will return before the
   operation is complete and must be called repetitively until the
   operation is complete, which is when the function returns true.  */
bool mmg_defrag(Block_Info* blocks, bool synchronous,
				unsigned max_iter)
{
	return true;
}

/* Access a single element in a block of data.  */
void* mmg_access(void* begin, unsigned offset, unsigned element_size)
{
	return (void*)val;
}

/* Accumulates multiple memory move operations and carries them out in
   a single, more efficient, instant.  */
void multi_memmove()
{
}

/* Interrupt and resume a memory move in process to allow cooperation
   with the move operation.  Any common move operations are merged and
   only a single memory move is executed.  */

/* Write a program to automatically insert co-opt points within a
   program to simulate pre-emptive multitasking.  Unlike real
   pre-emptive multitasking, this does not interfere with
   non-reentrant blocking functions, and it also doesn't burden the
   human programmer.  Note that precompiled external libraries may act
   as blocking points until they are recompiled with proper support.
   Only insert co-opt points in loops.  There's no point to inserting
   them elsewhere, except in the rare condition that thousands of
   people write enough unique code to slow down a 1 MHZ processor.
   How could that much code fit in such a small system?  */

/* Implement a re-entrant strtok().  */
/* As for other string functions, can I find something already in
   GLib?  */

/* Note that co-opt points should also implement a fast time-slicing
   mechanism.  If this proves to not be good enough, programmers can
   hand-optimize their code further.  */

/* Now I have to rewrite all of my code that uses macros not to
   include the last semicolon!  No I don't, I just thought I did at
   one point in time.  */
