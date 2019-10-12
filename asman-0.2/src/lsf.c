/* lsf.c -- "Fancy `ls'," format display into columns and show type
   indication character, similar to modern Unix `ls -F' commands, only
   simpler.

Copyright (C) 2017 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

/* The main caveat of this simple implementation is incompleteness in
   dealing with UTF-8 Unicode characters.  These end up being counted
   as two or more characters.  For CJK ideographs, this kind of works
   correctly.  For languages with only a small scattering of accented
   Roman characters, this causes only minimal error.  */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"

EA_TYPE(char);
EA_TYPE(char_array);
EA_TYPE(unsigned);

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* For simplicity, we don't allow command-line configuration of these
   options.  Rather, we set these to good defaults for the purpose of
   this tool system.  */
#define MIN_COL_WIDTH 8
static bool hide_dotfiles = false;
static bool sort_names = true;
static bool show_typechar = true;

static int file_cmp_func(const void *a, const void *b)
{
	const char_array *fa = (const char_array*)a;
	const char_array *fb = (const char_array*)b;
	unsigned ta_pos = fa->len - 2, tb_pos = fb->len - 2;
	char ta = fa->d[ta_pos], tb = fb->d[tb_pos];
	int result;
	/* Do not include the file type character while sorting.  */
	fa->d[ta_pos] = '\0';
	fb->d[tb_pos] = '\0';
	result = strcmp(fa->d, fb->d);
	fa->d[ta_pos] = ta;
	fb->d[tb_pos] = tb;
	return result;
}

int main(int argc, char *argv[])
{
	const char *path = ".";
	char *old_cwd = NULL;
	DIR *dir_handle;
	struct dirent *de;

	unsigned max_width = 80;
	unsigned max_columns = max_width / MIN_COL_WIDTH;
	char_array_array files;
	/* Horizontal lengths of columns.  The vertical length is the same
	   (`files_per_col') for all columns except the last one.  */
	unsigned_array col_lens;
	unsigned files_per_col;

	/* Read the file list, appending a file type specification
	   character as necessary.  */
	if (argc >= 2)
	{
		old_cwd = getcwd(NULL, 0);
		path = argv[1];
		if (chdir(path) != 0)
		{ xfree(old_cwd); return 1; }
	}
	dir_handle = opendir(".");
	if (dir_handle == NULL)
	{ xfree(old_cwd); return 1; }
	EA_INIT(files, 16);
	while (de = readdir(dir_handle))
	{
		unsigned name_len;
		struct stat sbuf;

		if (hide_dotfiles && de->d_name[0] == '.')
			continue;

		name_len = strlen(de->d_name);
		/* Note: We add 3 to the string length for (1) the file type
		   specification character, (2), the null character, (3) and
		   for keeping some reserve space.  */
		EA_INIT(files.d[files.len], name_len + 3);
		files.d[files.len].len = name_len + 2;
		strcpy(files.d[files.len].d, de->d_name);
		files.d[files.len].d[name_len] = ' ';
		files.d[files.len].d[name_len+1] = '\0';

		{ /* Replace odd whitespace characters with '?' so that they
			 don't mess with our table formatting on output.  Note
			 that this alters sorting order compared to the original
			 characters, but this is the same behavior as GNU
			 `ls'.  */
			char *pos = files.d[files.len].d;
			while (*pos)
			{
				if (*pos == '\t' || *pos == '\r' ||
					*pos == '\n' || *pos == '\f')
					*pos = '?';
				pos++;
			}
		}

		if (show_typechar && !lstat(de->d_name, &sbuf))
		{
			char type_char = ' ';
			if (S_ISDIR(sbuf.st_mode))
				type_char = '/';
			else if (S_ISLNK(sbuf.st_mode))
				type_char = '@';
			/* This indicator may be nice, but it is too complicated
			   for our purposes.  */
			/* else if ((sbuf.st_mode & (S_IEXEC | S_IXGRP | S_IXOTH)))
				type_char = '*'; */
			files.d[files.len].d[name_len] = type_char;
		}

		EA_ADD(files);
	}
	closedir(dir_handle);
	if (old_cwd != NULL)
	{ chdir(old_cwd); xfree(old_cwd); }

	if (sort_names)
		qsort(files.d, files.len, sizeof(char_array), file_cmp_func);

	{ /* 1. Start with the default number of columns.  */
		const char *term_cols = getenv("COLUMNS");
		if (term_cols != NULL)
		{
			int new_val = atoi(term_cols);
			if (new_val > 0)
				max_width = new_val;
		}
		max_columns = max_width / MIN_COL_WIDTH;
		EA_INIT(col_lens, max_columns + 1);
		col_lens.len = max_columns;
	}

	while (1)
	{
		unsigned total_width = 0;
		unsigned def_col_width = max_width / col_lens.len;
		unsigned i;
		unsigned j = 0;

		/* 2. Compute number of items per column.  */
		files_per_col = files.len / col_lens.len +
			((files.len % col_lens.len > 0) ? 1 : 0);

		/* If you get to only one column, just output that as-is.  */
		if (col_lens.len <= 1)
			break;

		/* 3. Compute horizontal lengths of the columns.  */
		for (i = 0; i < col_lens.len; i++)
		{
			unsigned col_end;
			if (j >= files.len)
			{
				/* The remaining columns are unused.  */
				col_lens.d[i] = 0;
				continue;
			}
			/* Note: Setting a column width greater than zero here
			   effectively sets a minimum column width.  Doing this
			   can help keep the layout within some tolerance of a
			   uniform grid.  */
			col_lens.d[i] = def_col_width / 2;
			for (col_end = j + files_per_col;
				 j < files.len && j < col_end; j++)
			{
				/* Note: `files.d[j].len' includes the null character.
				   We'll use this to our advantage so that we don't
				   need to add one for mandatory space between the
				   column starts and filename ends.  */
				col_lens.d[i] = MAX(col_lens.d[i], files.d[j].len);
			}
		}

		/* 4. Add up the horizontal length of all the columns.  */
		for (i = 0; i < col_lens.len; i++)
			total_width += col_lens.d[i];

		/* 5. If the total horizonal length exceeds the display width,
		   reduce the number of columns by one and repeat 2-5.  */
		if (total_width > max_width)
		{
			col_lens.len--;
			continue;
		}

		/* Remove trailing zero-size columns before further
		   processing.  */
		for (i = col_lens.len - 1; i > 0 && col_lens.d[i] == 0; i--)
			col_lens.len--;
		def_col_width = max_width / col_lens.len;

		/* If we have surplus space beyond the minimum required column
		   length, and we have more than one line, distribute that to
		   make short columns closer to an ideal uniform width.  */
		if (files_per_col > 1)
		{
			unsigned surplus_width = max_width - total_width;
			unsigned surplus_per_col = 0;
			unsigned total_neg_space = 0;
			for (i = 0; i < col_lens.len; i++)
			{
				if (col_lens.d[i] < def_col_width)
					total_neg_space += def_col_width - col_lens.d[i];
			}
			/* If there is more than enough space to fill the negative
			   space, distribute the remaining surplus evenly per
			   column.  */
			if (total_neg_space < surplus_width)
				surplus_per_col =
					(surplus_width - total_neg_space) / col_lens.len;
			for (i = 0; i < col_lens.len; i++)
			{
				col_lens.d[i] += surplus_per_col;
				if (col_lens.d[i] < def_col_width)
				{
					unsigned neg_space = def_col_width - col_lens.d[i];
					if (total_neg_space > surplus_width)
					{
						col_lens.d[i] +=
							surplus_width * neg_space / total_neg_space;
					}
					else
						col_lens.d[i] += neg_space;
				}
			}
		}
		break;
	}

	{ /* Write the file names to standard output row-wise.  */
		unsigned i = 0;
		while (i < files_per_col)
		{
			unsigned j;
			unsigned col_start = 0;
			unsigned cur_len = 0;
			for (j = 0; j < col_lens.len; j++)
			{
				unsigned index = files_per_col * j + i;
				if (index >= files.len)
					/* Skip empty space that should only occur in the
					   last column.  */
					goto col_end;
				/* Pad out to the column start if we are not there
				   already.  */
				while (cur_len < col_start)
				{ putchar(' '); cur_len++; }
				fputs(files.d[index].d, stdout);
				/* Subtract one to avoid counting the null
				   character.  */
				cur_len +=  files.d[index].len - 1;
			col_end:
				col_start += col_lens.d[j];
				if (j >= col_lens.len - 1)
					putchar('\n');
			}
			i++;
		}
	}

	{ /* Cleanup.  */
		unsigned i;
		for (i = 0; i < files.len; i++)
			EA_DESTROY(files.d[i]);
		EA_DESTROY(files);
		EA_DESTROY(col_lens);
	}
	return 0;
}
