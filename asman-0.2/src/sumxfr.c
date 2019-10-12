/* sumxfr.c -- Compare two checksum listings and generate file
   management commands.  `sumxfr' is short for "checksum extract and
   refresh."

Copyright (C) 2013, 2017 Andrew Makousky

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

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"
#include "exparray.h"
#include "cmdline.h"

typedef char* char_ptr;
EA_TYPE(char_ptr);

void display_help(const char *prog_name)
{
	printf("Usage: %s [options] SAVED-SUMS < NEW-SUMS > UP-CMDS\n",
		   prog_name);
	puts("\n"
"Generate file management commands from the differences between two\n"
"checksum listings.  The old sums are stored in a file whose name is\n"
"passed on the command line.  The new sums are passed in through\n"
"standard input.  Both sums are assumed to be sorted before program\n"
"execution.  If the sums are found to be out of order, this program\n"
"will signal an error and terminate.  Note that the output may contain\n"
"bogus commands, so you are recommended to pipe the output to `fmsimp'.\n"
"\n"
"Options:\n"
"  -l           Make hard links rather than copy files.\n"
"  -f           Process a list of file names without checksums.\n"
"  -c           Process output from `cksum' rather than `md5sum' or\n"
"               `sha*sum'.\n"
"  -h, --help   Display this help.\n"
"\n"
"Also note that the output script will always explicitly create any\n"
"necessary parent directories for any created or modified files.  If\n"
"you are concerned about long term usage of this utility leaving empty\n"
"directories laying around, just use the `delempty' tool to delete\n"
"empty directories.");
}

int main(int argc, char *argv[])
{
	int retval;
	bool make_links = false;
	bool only_fnames = false;
	bool cksum_output = false;
	char *sums_name = NULL;
	FILE *fsaved;
	char *new_sum = NULL, *old_sum = NULL;
	char *last_new_sum = NULL, *last_old_sum = NULL;
	unsigned sum_len = 0;
	unsigned fname_pos = 0;
	/* Source-destination pairs are used for `mv_files' and
	   `cp_files'.  */
	char_ptr_array rm_files;
	char_ptr_array mv_files;
	char_ptr_array touch_files;
	char_ptr_array cp_files;

	{ /* Parse the command line.  */
		bool end_opt_parse = false;
		bool invalid_cmdline = false;
		int i;
		for (i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-' && !end_opt_parse)
			{
				if (!strcmp(argv[i], "--help"))
				{
					display_help(argv[0]);
					return 0;
				}
				if (argv[i][1] == '-')
				{
					end_opt_parse = true;
					continue;
				}
				while (*(++argv[i]))
				{
					switch (*argv[i]) {
					case 'l': make_links = true; break;
					case 'f': only_fnames = true; break;
					case 'c': cksum_output = true; break;
					case 'h': display_help(argv[0]); return 0; }
				}
			}
			else if (sums_name == NULL)
				sums_name = argv[i];
			else
			{
				invalid_cmdline = true;
				break;
			}
		}
		if (invalid_cmdline || i == 1 || sums_name == NULL ||
			(only_fnames && cksum_output))
		{
			printf("%s: invalid command line\n", argv[0]);
			printf("Type `%s --help' for more information.\n", argv[0]);
			return 1;
		}
	}

	fsaved = fopen(sums_name, "r");
	if (fsaved == NULL)
	{
		fprintf(stderr, "%s: ", argv[0]);
		perror(sums_name);
		return 1;
	}

	/* Initialize.  */
	exp_getline(stdin, &new_sum);
	exp_getline(fsaved, &old_sum);
	if (!only_fnames)
	{
		char *space_pos;
		unsigned num_spaces = (cksum_output) ? 2 : 1;
		unsigned i;
		space_pos = new_sum;
		for (i = 0; i < num_spaces; i++)
		{
			space_pos = strchr(space_pos, ' ');
			if (space_pos == NULL)
			{
				fprintf(stderr, "%s: invalid checksum format\n", argv[0]);
				xfree(new_sum); xfree(old_sum);
				return 1;
			}
		}
		sum_len =  space_pos - new_sum;
		if (cksum_output)
			fname_pos = sum_len + 1;
		else
			fname_pos = sum_len + 2;
	}
	if (escape_newlines)
		fputs("NL='\n'\n", stdout);
	EA_INIT(rm_files, 16);
	EA_INIT(mv_files, 16);
	EA_INIT(cp_files, 16);
	EA_INIT(touch_files, 16);

	/* Compare the checksums and create file management commands.  */
	while (!feof(stdin) || !feof(fsaved))
	{
		int sum_cmp;
		if (only_fnames)
		{
			if (last_new_sum != NULL && last_old_sum != NULL && 
				(strcmp(last_new_sum, new_sum) > 0 ||
				 strcmp(last_old_sum, old_sum) > 0))
			{
				unsigned i;
				fprintf(stderr, "%s: unsorted input list\n", argv[0]);
				retval = 1; goto cleanup;
			}
			sum_cmp = strcmp(new_sum, old_sum);
		}
		else
		{
			if (last_new_sum != NULL && last_old_sum != NULL && 
				(strncmp(last_new_sum, new_sum, sum_len) > 0 ||
				 strncmp(last_old_sum, old_sum, sum_len) > 0))
			{
				unsigned i;
				fprintf(stderr, "%s: unsorted input list\n", argv[0]);
				retval = 1; goto cleanup;
			}
			sum_cmp = strncmp(new_sum, old_sum, sum_len);
		}

		if (sum_cmp < 0 || feof(fsaved))
		{
			/* A new file was created.  */
			char *new_name = new_sum + fname_pos;

			{ /* Create the directory for the file.  */
				char *dirname = new_name;
				char *new_argv[] = { "mkdir", "-p", NULL };
				char *splice_pos = strrchr(dirname, '/');
				if (splice_pos == NULL)
					splice_pos = dirname + strlen(dirname);
				STR_BEGIN_SPLICE(splice_pos);
				new_argv[2] = dirname;
				write_cmdline(stdout, 3, new_argv, 0);
				STR_END_SPLICE();
			}

			/* Store the update files.  */
			if (last_new_sum != NULL && sum_len > 0 &&
				!strncmp(last_new_sum, new_sum, sum_len))
			{
				char *orig_name = last_new_sum + fname_pos;
				EA_APPEND(cp_files, xstrdup(orig_name));
				EA_APPEND(cp_files, xstrdup(new_name));
			}
			else
				EA_APPEND(touch_files, xstrdup(new_name));
			/* Keep going for any other new files.  */
			xfree(last_new_sum); last_new_sum = new_sum;
			exp_getline(stdin, &new_sum);
		}
		else if (sum_cmp == 0)
		{
			/* A file may have been renamed.  */
			char *old_name = old_sum + fname_pos;
			char *new_name = new_sum + fname_pos;
			if (strcmp(old_name, new_name))
			{
				{ /* Create the directory for the file.  */
					char *dirname = new_name;
					char *new_argv[] = { "mkdir", "-p", NULL };
					STR_BEGIN_SPLICE(strrchr(dirname, '/'));
					new_argv[2] = dirname;
					STR_END_SPLICE();
				}
				EA_APPEND(mv_files, xstrdup(old_name));
				EA_APPEND(mv_files, xstrdup(new_name));
			}
			/* Keep going for any other renamed files.  */
			xfree(last_new_sum); last_new_sum = new_sum;
			exp_getline(stdin, &new_sum);
			xfree(last_old_sum); last_old_sum = old_sum;
			exp_getline(fsaved, &old_sum);
		}
		else /* if (sum_cmp > 0 || feof(stdin)) */
		{
			/* An old file was deleted.  */
			char *old_name = old_sum + fname_pos;
			EA_APPEND(rm_files, xstrdup(old_name));
			/* Keep going for any other deleted files.  */
			xfree(last_old_sum); last_old_sum = old_sum;
			exp_getline(fsaved, &old_sum);
		}
	}

	{ /* Write out the file management commands.  */
		unsigned i = 0;
		while (i < rm_files.len)
		{
			char *new_argv[] = { "rm", NULL };
			new_argv[1] = rm_files.d[i++];
			write_cmdline(stdout, 2, new_argv, 0);
			xfree(new_argv[1]);
		}
		i = 0;
		while (i < mv_files.len)
		{
			char *new_argv[] = {"mv", NULL, NULL };
			new_argv[1] = mv_files.d[i++];
			new_argv[2] = mv_files.d[i++];
			write_cmdline(stdout, 3, new_argv, 0);
			xfree(new_argv[1]);
			xfree(new_argv[2]);
		}
		i = 0;
		while (i < touch_files.len)
		{
			char *new_argv[] = { "touch", NULL };
			new_argv[1] = touch_files.d[i++];
			write_cmdline(stdout, 2, new_argv, 0);
			xfree(new_argv[1]);
		}
		i = 0;
		if (make_links)
			while (i < cp_files.len)
			{
				char *new_argv[] = {"ln", NULL, NULL };
				new_argv[1] = cp_files.d[i++];
				new_argv[2] = cp_files.d[i++];
				write_cmdline(stdout, 3, new_argv, 0);
				xfree(new_argv[1]);
				xfree(new_argv[2]);
			}
		else
			while (i < cp_files.len)
			{
				char *new_argv[] = {"cp", "-p", NULL, NULL };
				new_argv[2] = cp_files.d[i++];
				new_argv[3] = cp_files.d[i++];
				write_cmdline(stdout, 4, new_argv, 0);
				xfree(new_argv[2]);
				xfree(new_argv[3]);
			}
	}
	retval = 0;

cleanup:
	xfree(new_sum); xfree(old_sum);
	xfree(last_new_sum); xfree(last_old_sum);
	EA_DESTROY(rm_files);
	EA_DESTROY(mv_files);
	EA_DESTROY(touch_files);
	EA_DESTROY(cp_files);
	fclose(fsaved);
	return retval;
}
