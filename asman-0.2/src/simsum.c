/* simsum.c -- Simulate file management commands by transforming
   checksum lists.  This simulator does not include special support
   for hard link or symbolic link simulation.

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
#include <stdlib.h>
#include <errno.h>

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"
#include "exparray.h"
#include "cmdline.h"

struct FileEntry_tag
{
	char *name;
	char *cksum;
};
typedef struct FileEntry_tag FileEntry;

EA_TYPE(FileEntry);

char *prog_name;
unsigned cksum_len = 0;
FileEntry_array file_database;

FileEntry *fd_find_file(char *name);
FileEntry *fd_first_dirent(FileEntry *dir_dummy);
bool fd_is_dirent(FileEntry *dir_dummy, FileEntry *cur_dir_entry);
FileEntry *fd_next_dirent(FileEntry *dir_dummy, FileEntry *cur_dir_entry);
void fd_insert(char *name, char *cksum);
int proc_null(int argc, char *argv[]);
int proc_touch(int argc, char *argv[]);
int proc_cp(int argc, char *argv[]);
int proc_mv(int argc, char *argv[]);
int proc_rm(int argc, char *argv[]);

static int fd_cmp_func(const void *a, const void *b)
{
	const FileEntry *fe1 = (FileEntry*)a;
	const FileEntry *fe2 = (FileEntry*)b;
	return strcmp(fe1->name, fe2->name);
}

int main(int argc, char *argv[])
{
	int retval;
	char *sum_file;
#define NUM_CMDS 7
	const char *commands[NUM_CMDS] =
		{ "touch", "cp", "mv", "rm", "ln", "mkdir", "rmdir" };
	const CmdFunc cmdfuncs[NUM_CMDS] = { proc_touch, proc_cp, proc_mv,
					 proc_rm, proc_cp, proc_null, proc_null };

	if (argc == 2 && (!strcmp(argv[1], "-h") ||
					  !strcmp(argv[1], "--help")))
	{
		printf("Usage: %s [ORIG-SUMS] < COMMANDS > NEW-SUMS\n", argv[0]);
		puts(
"Simulate file management commands by transforming a checksum list.\n"
"The `touch' commands in the input must be augmented to give a `--sum'\n"
"option specifying the new checksum.");
		return 0;
	}

	if (argc == 3 && !strcmp(argv[1], "--"))
		sum_file = argv[2];
	else if (argc > 2)
	{
		fprintf(stderr, "%s: invalid command line\n", argv[0]);
		fprintf(stderr, "Type `%s --help' for more information.\n", argv[0]);
		return 1;
	}
	else if (argc == 2)
		sum_file = argv[1];
	else
		sum_file = NULL;
	prog_name = argv[0];

	EA_INIT(file_database, 16);

	if (sum_file != NULL)
	{
		/* Read the checksum file.  */
		FILE *fp = fopen(sum_file, "r");
		char *line = NULL;
		if (fp == NULL)
		{
			fprintf(stderr, "%s: %s: %s\n", prog_name, sum_file,
					strerror(errno));
			retval = 1; goto cleanup;
		}
		while (exp_getline(fp, &line) != EOF)
		{
			unsigned line_len = strlen(line);
			if (cksum_len == 0)
			{
				char *cur_pos = line;
				while (*cur_pos != '\0' && *cur_pos != ' ')
					cur_pos++;
				cksum_len = cur_pos - line;
				if (cksum_len == 0)
				{
					fprintf(stderr, "%s: invalid checksum file.\n",
							prog_name);
					retval = 1; goto cleanup;
				}
			}

			if (line_len <= cksum_len + 2)
			{
				fprintf(stderr, "%s: invalid checksum file.\n", prog_name);
				retval = 1; goto cleanup;
			}

			line[cksum_len] = '\0';
			file_database.d[file_database.len].name =
				xstrdup(&line[cksum_len+2]);
			file_database.d[file_database.len].cksum = xstrdup(line);
			EA_ADD(file_database);

			xfree(line);
		}
		if (line != NULL)
			xfree(line);
		fclose(fp);

		qsort(file_database.d, file_database.len, sizeof(FileEntry),
			  fd_cmp_func);
	}

	{ /* Verify that there are no duplicate filenames.  */
		unsigned i;
		for (i = 1; i < file_database.len; i++)
		{
			if (!strcmp(file_database.d[i-1].name, file_database.d[i].name))
			{
				fprintf(stderr,
						"%s: duplicate filenames in checksum list.\n",
						prog_name);
				retval = 1; goto cleanup;
			}
		}
	}

	/* Interpret the command script.  */
	retval = proc_cmd_dispatch(stdin, NUM_CMDS, commands, cmdfuncs, true);
	if (retval != 0)
		goto cleanup;

	{ /* Output the new checksum listing.  */
		unsigned i;
		for (i = 0; i < file_database.len; i++)
		{
			printf("%s  %s\n", file_database.d[i].cksum,
				   file_database.d[i].name);
		}
	}

	retval = 0;

cleanup:
	{
		unsigned i;
		for (i = 0; i < file_database.len; i++)
		{
			xfree(file_database.d[i].name);
			xfree(file_database.d[i].cksum);
		}
		EA_DESTROY(file_database);
	}
	return retval;
}

/* Search for the record in the filename database that contains the
   given filename.  */
FileEntry *fd_find_file(char *name)
{
	FileEntry temp_entry;
	temp_entry.name = name;
	return (FileEntry*)bsearch(&temp_entry, file_database.d,
							   file_database.len, sizeof(FileEntry),
							   fd_cmp_func);
}

/* prefix bsearch compare: check if "key" is a prefix of "aentry". */
static int prefix_bs_cmp(const void *key, const void *aentry)
{
	const FileEntry *fe1 = (const FileEntry*)key;
	const FileEntry *fe2 = (const FileEntry*)aentry;

	char *prefix_str = fe1->name, *path_str = fe2->name;
	unsigned prefix_len = strlen(prefix_str);
	unsigned path_len = strlen(path_str);

	int cmpres = strncmp(prefix_str, path_str, prefix_len);

	if (cmpres == 0)
	{
		if (path_len >= prefix_len &&
			(path_str[prefix_len] == '/' ||
			 (path_str[prefix_len-1] == '/' &&
			  prefix_str[prefix_len-1] == '/')))
			return 0;
		else if (path_len > prefix_len)
			return (int)'/' - (int)(path_str[prefix_len]);
	}

	return cmpres;
}

/* Search for the file that is the first directory entry of the given
   directory.  `dir_dummy' is a FileEntry whose `name' is filled in
   with the name of the directory.  */
FileEntry *fd_first_dirent(FileEntry *dir_dummy)
{
	FileEntry *cur_dir_entry;

	cur_dir_entry = (FileEntry*)bsearch_ordered(
		dir_dummy, file_database.d, file_database.len,
		sizeof(FileEntry), prefix_bs_cmp);
	if (cur_dir_entry == NULL)
		return NULL;

	/* The bsearch entry may not be the first directory entry, so
	   search backward until the first one is reached.  */
	while (cur_dir_entry > file_database.d)
	{
		if (!prefix_bs_cmp(dir_dummy, cur_dir_entry - 1))
			cur_dir_entry--;
		else
			break;
	}

	return cur_dir_entry;
}

/* Check if `cur_dir_entry' is a directory entry (immediate child) of
   `dir_dummy'.  */
bool fd_is_dirent(FileEntry *dir_dummy, FileEntry *cur_dir_entry)
{
	return !prefix_bs_cmp(dir_dummy, cur_dir_entry);
}

/* Get the next directory entry in a directory, or return NULL if a
   directory has no more entries.  */
FileEntry *fd_next_dirent(FileEntry *dir_dummy, FileEntry *cur_dir_entry)
{
	cur_dir_entry += 1;
	if (cur_dir_entry - file_database.d < file_database.len &&
		fd_is_dirent(dir_dummy, cur_dir_entry))
		return cur_dir_entry;
	return NULL;
}

static int fd_insert_cmp_func(const void *key, const void *aentry)
{
	const FileEntry *fe = (const FileEntry*)aentry;
	return strcmp((char*)key, fe->name);
}

/* Locate where in the filename database to insert the given
   filename-checksum pair and insert it.  */
void fd_insert(char *name, char *cksum)
{
	bool element_exists;
	unsigned ins_pos = bs_insert_pos(name, file_database.d, file_database.len,
									 sizeof(FileEntry), fd_insert_cmp_func,
									 &element_exists);

	if (element_exists)
	{
		/* Do not insert the filename.  Replace the checksum
		   instead.  */
		strncpy(file_database.d[ins_pos].cksum, cksum, cksum_len);
		xfree(name);
		xfree(cksum);
		return;
	}
	else
	{
		/* Insert the element.  */
		EA_INS(file_database, ins_pos);
		file_database.d[ins_pos].name = name;
		file_database.d[ins_pos].cksum = cksum;
	}
}

/* Pass a command through without any special processing.  */
int proc_null(int argc, char *argv[])
{
	return 0;
}

/* Process a `touch' command.  "Touch" is the generic term for any
   kind of command that requires creating or updating a file or
   directory.  Touching directories with `simsum' does not work
   since the checksum of an individual file must be specified.  */
int proc_touch(int argc, char *argv[])
{
	unsigned i;
	char *src;
	char *new_sum = NULL;
	FileEntry *entry;
	unsigned index;

	if (argc < 4)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-1];
	for (i = 1; i < argc - 1; i++)
	{
		if (!strncmp("--sum", argv[i], 5) && i < argc - 2)
		{
			new_sum = argv[i+1];
			break;
		}
	}
	if (new_sum == NULL)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}

	/* Change the appropriate checksum.  */
	entry = fd_find_file(src);
	if (entry != NULL)
		strncpy(entry->cksum, new_sum, cksum_len);
	else
	{
		if (cksum_len == 0)
			cksum_len = strlen(new_sum);
		fd_insert(xstrdup(src), xstrdup(new_sum));
	}

	return 0;
}

/* Process a copy command.  All command-line options must come first.
   Command-line options are ignored, and if a file is a directory, the
   copy command is assumed to be recursive as if an `-R' switch were
   specified.  `-p' and `-T are implied, whether or not they are
   actually specified.  Only one source file may be specified.
   Symbolic links to directories are not supported.  */
int proc_cp(int argc, char *argv[])
{
	char *src, *dest;
	FileEntry *src_entry;
	unsigned index;

	if (argc < 3)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-2];
	dest = argv[argc-1];

	/* Add the new destination filename.  If the destination already
	   exists, then the checksum will be updated accordingly.  */
	src_entry = fd_find_file(src);
	if (src_entry != NULL)
		fd_insert(xstrdup(dest), xstrdup(src_entry->cksum));
	else
	{
		/* Attempt to copy directory trees.  */
		FileEntry dir_dummy = { src, "" };
		FileEntry *cur_dir_entry = fd_first_dirent(&dir_dummy);
		unsigned src_len = strlen(src);
		unsigned dest_len = strlen(dest);
		if (cur_dir_entry == NULL)
		{
			fprintf(stderr,
					"stdin:%u: error: no such file or directory: %s\n",
					last_line_num, src);
			return 1;
		}

		if (src[src_len-1] == '/')
			src[--src_len] = '\0';
		if (dest[dest_len-1] == '/')
			dest[--dest_len] = '\0';

		/* Don't try to copy a directory into itself.  */
		if (!strncmp(src, dest, src_len))
		{
			fprintf(stderr, "stdin:%u: error: cannot copy a "
					"directory, `%s', into itself, `%s'\n",
					last_line_num, src, dest);
			return 1;
		}

		do
		{
			char *new_name = (char*)xmalloc(sizeof(char) *
				(dest_len + (strlen(cur_dir_entry->name) - src_len) + 1));
			strcpy(new_name, dest);
			new_name[dest_len] = '/';
			strcpy(&new_name[dest_len+1], cur_dir_entry->name + src_len + 1);

			fd_insert(new_name, xstrdup(cur_dir_entry->cksum));
			cur_dir_entry = fd_next_dirent(&dir_dummy, cur_dir_entry);
		} while (cur_dir_entry != NULL);
	}
	return 0;
}

/* Process a move command.  All command-line options must come first.
   Command-line options are ignored.  `-T' is implied, whether or not
   it is actually specified.  Only one source file may be
   specified.  */
int proc_mv(int argc, char *argv[])
{
	char *src, *dest;
	unsigned src_len, dest_len;
	FileEntry *src_entry, *dest_entry;

	if (argc < 3)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-2];
	dest = argv[argc-1];

	{
		/* Remove any trailing slashes from the names.  */
		src_len = strlen(src);
		dest_len = strlen(dest);
		if (src[src_len-1] == '/')
			src[--src_len] = '\0';
		if (dest[dest_len-1] == '/')
			dest[--dest_len] = '\0';

		/* Verify that the source and destination are not identical.  */
		if (!strcmp(src, dest))
		{
			fprintf(stderr, "stdin:%u: error: source and destination "
					"names are identical.\n", last_line_num, argv[0]);
			return 1;
		}

		/* Don't try to move a directory into itself.  */
		if (!strncmp(src, dest, src_len))
		{
			fprintf(stderr, "stdin:%u: error: cannot move a "
					"directory, `%s', into itself, `%s'\n",
					last_line_num, src, dest);
			return 1;
		}
	}

	src_entry = fd_find_file(src);
	if (src_entry != NULL)
	{
		/* Delete the source filename and add the destination filename
	       with the same checksum.  */
		char *cksum = src_entry->cksum;
		unsigned src_index = src_entry - file_database.d;
		xfree(src_entry->name);
		EA_REMOVE(file_database, src_index);
		fd_insert(xstrdup(dest), cksum);
	}
	else
	{
		/* Attempt to move directory trees.  */
		FileEntry dir_dummy = { src, "" };
		FileEntry *cur_dir_entry = fd_first_dirent(&dir_dummy);
		if (cur_dir_entry == NULL)
		{
			fprintf(stderr,
					"stdin:%u: error: no such file or directory: %s\n",
					last_line_num, src);
			return 1;
		}

		/* Move the directory entries by changing the names of the
		   existing entries.  */
		do
		{
			char *new_name = (char*)xmalloc(sizeof(char) *
				(dest_len + (strlen(cur_dir_entry->name) - src_len) + 1));
			strcpy(new_name, dest);
			new_name[dest_len] = '/';
			strcpy(&new_name[dest_len+1], cur_dir_entry->name + src_len + 1);

			xfree(cur_dir_entry->name);
			cur_dir_entry->name = new_name;
			cur_dir_entry = fd_next_dirent(&dir_dummy, cur_dir_entry);
		} while (cur_dir_entry != NULL);

		/* Sort in the new entries.  */
		qsort(file_database.d, file_database.len, sizeof(FileEntry),
			  fd_cmp_func);
	}

	return 0;
}

/* Process a remove command.  Command-line switches must come first.
   Only one path name may be specified on the command-line.  `-rf' is
   implied, whether or not it is actually specified.  */
int proc_rm(int argc, char *argv[])
{
	char *src;
	FileEntry *entry;

	if (argc < 2)
	{
		fprintf(stderr, "stdin:%u: error: invalid command line for `%s'\n",
				last_line_num, argv[0]);
		return 1;
	}
	src = argv[argc-1];

	/* Remove the checksum from the list.  */
	entry = fd_find_file(src);
	if (entry != NULL)
	{
		/* Delete a single file.  */
		unsigned index = entry - file_database.d;
		xfree(entry->name);
		xfree(entry->cksum);
		EA_REMOVE(file_database, index);
	}
	else
	{
		/* Attempt to delete directory trees.  */
		FileEntry dir_dummy = { src, "" };
		FileEntry *cur_dir_entry = fd_first_dirent(&dir_dummy);
		unsigned start_idx, end_idx;
		if (cur_dir_entry == NULL)
		{
			fprintf(stderr,
					"stdin:%u: error: no such file or directory: %s\n",
					last_line_num, src);
			return 1;
		}

		start_idx = cur_dir_entry - file_database.d;
		end_idx = start_idx;
		do
		{
			xfree(cur_dir_entry->name);
			xfree(cur_dir_entry->cksum);
			end_idx++;
			cur_dir_entry = fd_next_dirent(&dir_dummy, cur_dir_entry);
		} while (cur_dir_entry != NULL);
		EA_REMOVE_MULT(file_database, start_idx, end_idx - start_idx);
	}

	return 0;
}
