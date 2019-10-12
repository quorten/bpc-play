/* lsrize.c -- Convert a file name listing to a `ls -R' listing.
   Since file names are separated by newlines, file names cannot have
   newlines in them.  */

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "strheap.h"

typedef char* char_ptr;
EA_TYPE(char_ptr);

struct DirEntry_tag
{
	char *name;
	char_ptr_array files;
};
typedef struct DirEntry_tag DirEntry;
EA_TYPE(DirEntry);

DirEntry_array dirs;

int prefix_cmp(const void *key, const void *aentry)
{
	const char *prefix = (const char*)key;
	const DirEntry *dir = (const DirEntry*)aentry;
	return strcmp(prefix, dir->name);
}

int main(int argc, char *argv[])
{
	char *newpath;

	if (argc != 1)
	{
		printf("Usage: %s < FILE-LIST > LS-R-LIST\n", argv[0]);
		puts(
"Convert a file name listing (such as from `find .') to an `ls -R' style\n"
"listing.  Since file names are separated by newlines, file names cannot\n"
"have newlines in them.");
		return 0;
	}

	EA_INIT(dirs, 16);

	/* Read the filename.  */
	while (exp_getline(stdin, &newpath) != EOF)
	{
		char *newprefix;
		char *filename;

		filename = strrchr(newpath, '/');
		if (filename != NULL)
		{
			*filename++ = '\0';
			if (filename[0] == '\0') /* trailing slash on path */
			{
				while (filename > newpath && *filename != '/')
					filename--;
				if (filename == newpath)
					filename = NULL;
				else
					*filename++ = '\0';
			}
		}
		newprefix = newpath;

		{
			bool element_exists;
			unsigned i;
			i = bs_insert_pos(newprefix, dirs.d, dirs.len, sizeof(DirEntry),
							  prefix_cmp, &element_exists);
			if (!element_exists)
			{
				EA_INS(dirs, i);
				dirs.d[i].name = xstrdup(newprefix);
				EA_INIT(dirs.d[i].files, 16);
			}
			if (filename != NULL)
				EA_APPEND(dirs.d[i].files, xstrdup(filename));
		}

		xfree(newpath); newpath = NULL;
	}
	xfree(newpath);

	{ /* Output the `ls -R' listing.  */
		int i;
		for (i = 0; i < dirs.len; i++)
		{
			int j;
			fputs(dirs.d[i].name, stdout);
			fputs(":\n", stdout);
			xfree(dirs.d[i].name);
			for (j = 0; j < dirs.d[i].files.len; j++)
			{
				puts(dirs.d[i].files.d[j]);
				xfree(dirs.d[i].files.d[j]);
			}
			EA_DESTROY(dirs.d[i].files);
			if (i != dirs.len - 1)
				fputs("\n", stdout);
		}

	}

	EA_DESTROY(dirs);
	return 0;
}
