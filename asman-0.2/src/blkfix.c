/* blkfix.c -- Merge together `dd' disk images that have varying
   blocks missing due to errors to get an error-minimal image.  The
   accumulator file is overwritten, so start by making a copy for your
   accumulator file!

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
#include <string.h>

unsigned long long g_bytes_accum = 0;

int accum_block(unsigned long block_size,
				unsigned char *buf_zero,
				unsigned char *buf_in,
				unsigned char *buf_out,
				FILE *fout,
				FILE *fin,
				unsigned long long idx)
{
	int retval = 0;
	if (fread(buf_in, 1, block_size, fin) != block_size)
	{ retval = 1; goto cleanup; }
	if (fread(buf_out, 1, block_size, fout) != block_size)
	{ retval = 1; goto cleanup; }
	if (memcmp(buf_out, buf_zero, block_size) == 0 &&
		memcmp(buf_in, buf_zero, block_size) != 0)
	{
		/* Our output block is zero, and our input block is not zero,
		   so write it.  */
		if (fseeko64(fout, -(long)block_size, SEEK_CUR) != 0)
		{ retval = 1; goto cleanup; }
		if (fwrite(buf_in, 1, block_size, fout) != block_size)
		{ retval = 1; goto cleanup; }
		g_bytes_accum += block_size;
	}
	else
	{
		/* Since we already have a block in the output, we need not
		   write anything.  However, we must verify that both the
		   source block is also the same or else we've got bigger
		   problems.  But, if our source block is zero, then we simply
		   ignore it, i.e. it failed to be read due to error.  */
		if (memcmp(buf_in, buf_out, block_size) != 0 &&
			memcmp(buf_in, buf_zero, block_size) != 0)
		{
			fprintf(stderr,
					"Error: Block %llu is inconsistent!\n", idx);
			// retval = 1; goto cleanup;
		}
	}
cleanup:
	return retval;
}

int main(int argc, char *argv[])
{
	unsigned long block_size;
	char *accum_name;
	char *src_name;
	unsigned long long src_size;
	unsigned long long accum_size;

	struct stat64 s64buf;

	if (argc != 4)
	{
		fprintf(stderr, "Usage: %s BLKSIZE ACCUM SRC\n", argv[0]);
		return 1;
	}
	block_size = atoi(argv[1]);
	accum_name = argv[2];
	src_name = argv[3];

	if (stat64(src_name, &s64buf) == -1)
		return 1;
	src_size = s64buf.st_size;

	if (stat64(accum_name, &s64buf) == -1)
	{
		/* Create a blank accumulation file.  */
		FILE *fp = fopen(accum_name, "w");
		if (fp == NULL)
			return 1;
		if (fclose(fp) == EOF)
			return 1;
		accum_size = 0;
	}
	else
		accum_size = s64buf.st_size;

	if (accum_size < src_size)
	{
		truncate64(accum_name, src_size);
		accum_size = src_size;
	}

	/* Now, run through the source file a block at a time, and paste
	   it into the destination if the destination block is null.  */
	{ /* else, we have to copy the tail blocks.  */
		int retval = 0;
		unsigned long long num_blocks = src_size / block_size;
		unsigned long mod_tail = src_size % block_size;
		unsigned long long i = 0;
		unsigned char *buf_zero = NULL;
		unsigned char *buf_in = NULL;
		unsigned char *buf_out = NULL;
		FILE *fout = NULL;
		FILE *fin = NULL;

		buf_zero = (unsigned char*)malloc(block_size);
		if (buf_zero == NULL)
			return 1;
		buf_in = (unsigned char*)malloc(block_size);
		if (buf_in == NULL)
			return 1;
		buf_out = (unsigned char*)malloc(block_size);
		if (buf_out == NULL)
			return 1;

		memset(buf_zero, 0, block_size);
		fout = fopen(accum_name, "r+");
		fin = fopen(src_name, "r");
		if (fout == NULL || fin == NULL)
		{ retval = 1; goto cleanup; }

		/* Continue the block accumulation until we reach the end.  */
		for (i = 0; i < num_blocks; i++)
		{
			if (accum_block(block_size, buf_zero,
							buf_in, buf_out, fout, fin, i) != 0)
			{ retval = 1; goto cleanup; }
		}

		/* Accumulate the modulo-block tail, if applicable.  */
		if (mod_tail)
		{
			if (accum_block(mod_tail, buf_zero,
							buf_in, buf_out, fout, fin, i) != 0)
			{ retval = 1; goto cleanup; }
		}

		retval = 0;
	cleanup:
		fprintf(stderr, "%llu bytes accumulated\n", g_bytes_accum);
		free(buf_zero);
		free(buf_in);
		free(buf_out);
		fclose(fout);
		fclose(fin);
		return retval;
	}

	return 0;
}
