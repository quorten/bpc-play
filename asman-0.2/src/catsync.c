/* catsync.c -- Kind of like rsync, but simpler.  Only synchronize one
   file to another by either truncating or appending to the end of the
   destination file.

Copyright (C) 2019 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/* If this is what it takes to get `struct stat64'...  */
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char *src_name;
	char *dest_name;
	unsigned long long src_size;
	unsigned long long dest_size;
	unsigned long block_size;

	struct stat64 s64buf;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s SRC DEST\n", argv[0]);
		return 1;
	}
	src_name = argv[1];
	dest_name = argv[2];

	if (stat64(src_name, &s64buf) == -1)
		return 1;
	src_size = s64buf.st_size;

	/* Compute a somewhat optimal buffer block size for copying blocks
	   from the file.  */
	block_size = s64buf.st_blksize * 32;

	if (stat64(dest_name, &s64buf) == -1)
	{
		/* Create an empty destination file.  */
		FILE *fp = fopen(dest_name, "w");
		if (fp == NULL)
			return 1;
		if (fclose(fp) == EOF)
			return 1;
		dest_size = 0;
	}
	else
		dest_size = s64buf.st_size;

	if (dest_size > src_size)
	{
		truncate64(dest_name, src_size);
		return 0;
	}
	else if (dest_size == src_size)
		return 0;

	{ /* else, we have to copy the tail blocks.  */
		int retval = 0;
		unsigned long mod_lead = block_size - (dest_size % block_size);
		unsigned long first_blk_sz = mod_lead;
		unsigned long long tail_size = src_size - (dest_size + mod_lead);
		unsigned long long num_blocks = tail_size / block_size;
		unsigned long mod_tail = tail_size % block_size;
		unsigned long long i = 0;
		// unsigned long long bytes_cp = 0;
		unsigned char *buffer = NULL;
		FILE *fin = NULL;
		FILE *fout = NULL;

		buffer = (unsigned char*)malloc(block_size);
		if (buffer == NULL)
			return 1;

		fin = fopen(src_name, "r");
		fout = fopen(dest_name, "a");
		if (fin == NULL || fout == NULL)
		{ retval = 1; goto cleanup; }
		if (fseeko64(fin, dest_size, SEEK_SET) != 0)
		{ retval = 1; goto cleanup; }

		/* First, copy the modulo-block leader.  If the source file is
		   no larger than this, then we simply copy the remainder of
		   the source file in one block and then we are done.  */
		if (dest_size + mod_lead > src_size)
			first_blk_sz = src_size - dest_size;
		if (fread(buffer, 1, first_blk_sz, fin) != first_blk_sz)
		{ retval = 1; goto cleanup; }
		if (fwrite(buffer, 1, first_blk_sz, fout) != first_blk_sz)
		{ retval = 1; goto cleanup; }
		// bytes_cp += first_blk_sz;
		if (first_blk_sz < mod_lead)
		{ retval = 0; goto cleanup; }

		/* Now, continue the block copy until we reach the end.  */
		for (i = 0; i < num_blocks; i++)
		{
			if (fread(buffer, 1, block_size, fin) != block_size)
			{ retval = 1; goto cleanup; }
			if (fwrite(buffer, 1, block_size, fout) != block_size)
			{ retval = 1; goto cleanup; }
			// bytes_cp += block_size;
		}

		/* Copy the modulo-block tail, if applicable.  */
		if (mod_tail)
		{
			if (fread(buffer, 1, mod_tail, fin) != mod_tail)
			{ retval = 1; goto cleanup; }
			if (fwrite(buffer, 1, mod_tail, fout) != mod_tail)
			{ retval = 1; goto cleanup; }
			// bytes_cp += mod_tail;
		}

		retval = 0;
	cleanup:
		// fprintf(stderr, "%llu bytes copied\n", bytes_cp);
		free(buffer);
		fclose(fin);
		fclose(fout);
		return retval;
	}

	return 0;
}
