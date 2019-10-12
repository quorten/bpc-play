/* duhcs.c -- Simple tool that is similar to `du -hcs'.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/* If this is what it takes to get `struct stat64'...  */
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bool.h"
#include "xmalloc.h"
#include "dirwalk.h"
#include "cmdline.h"

static bool round_to_block = true;

static unsigned block_size;
static unsigned long long total_size;

unsigned long long round_blksize(unsigned long long size,
								 unsigned block_size);
void size_disp_h(unsigned long long size, double *disp_num, char *suffix);
static DIR_STATUSCODE duhcs_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf);

int main(int argc, char *argv[])
{
	int retval = 0;
	DirNode init = { ".", 1, NULL };
	pre_search_hook = duhcs_hook;
	post_search_hook = NULL;
	total_size = 0;
	{ /* Get the block size.  */
		struct stat sbuf;
		if (stat(".", &sbuf) == -1)
			return 1;
		block_size = sbuf.st_blksize;
	}

	if (argc <= 1)
	{
		if (search_dir(&init) >= DIR_ERRORS)
			return 1;
	}
	else
	{
		char *old_cwd = getcwd(NULL, 0);
		unsigned i;
		for (i = 1; i < argc; i++)
		{
			struct stat64 s64buf;
			if (stat64(argv[i], &s64buf) == -1)
			{ retval = 1; continue; }
			if (S_ISREG(s64buf.st_mode))
			{
				unsigned long long size = s64buf.st_size;
				if (round_to_block)
					size = round_blksize(size, block_size);
				total_size += size;
			}
			else if (S_ISDIR(s64buf.st_mode))
			{
				chdir(argv[i]);
				if (search_dir(&init) >= DIR_ERRORS)
					retval = 1;
				chdir(old_cwd);
			}
		}
		xfree (old_cwd);
	}

	{
		double disp_num;
		char suffix;
		/* printf("%lld total\n", total_size); */
		size_disp_h(total_size, &disp_num, &suffix);
		if (suffix != ' ')
			printf("%.3g%c total\n", disp_num, suffix);
		else
			printf("%g total\n", disp_num);
	}

	return retval;
}

unsigned long long round_blksize(unsigned long long size,
								 unsigned block_size)
{
	unsigned blksize_mask = block_size - 1;
	unsigned char remainder = (size & blksize_mask) > 0;
	return (size & ~(unsigned long long)blksize_mask) +
		(remainder ? block_size : 0);
}

/* Compute "human-readable" size display.  The results are stored in
   the `disp_num' and `suffix' pointers passed in.  If there is no
   suffix to be displayed, a space character is returned in
   `suffix'.  */
void size_disp_h(unsigned long long size, double *disp_num, char *suffix)
{
#define NUM_SUFFIX 9
	char scale_suffixes[NUM_SUFFIX] =
		{ ' ', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
	double calc_size;
	unsigned suffix_idx;
	calc_size = total_size;
	for (suffix_idx = 0; suffix_idx < NUM_SUFFIX &&
			 calc_size >= 1024; suffix_idx++)
		calc_size /= 1024;

	*disp_num = calc_size;
	*suffix = scale_suffixes[suffix_idx];
}

static DIR_STATUSCODE duhcs_hook(
	const DirNode *node, unsigned *num_files,
	const char *filename, const struct stat *sbuf)
{
	if (S_ISREG(sbuf->st_mode))
	{
		struct stat64 s64buf;
		if (stat64(filename, &s64buf) == 0)
		{
			unsigned long long size = s64buf.st_size;
			if (round_to_block)
				size = round_blksize(size, block_size);
			total_size += size;
		}
		/* else */
			/* error */
			/* total_size += sbuf->st_size; */
	}

	return DIR_OK;
}
