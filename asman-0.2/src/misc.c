/* Miscellaneous, but important, common functions.

Copyright (C) 2013, 2018 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#include <stdio.h>
#include <string.h>

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"

/* Get a line and store it in a dynamically-allocated memory block.
   The pointer to this block is returned in `out_ptr'.  The newline
   character read from the line is not stored in the output string.
   EOF is returned when EOF is encountered during reading; otherwise,
   1 is returned.  */
int exp_getline(FILE *fp, char **out_ptr)
{
	char *buffer;
	char *line_data = NULL;
	unsigned line_len = 0;
	do
	{
		line_data = (char*)xrealloc(line_data, line_len + 1024);
		buffer = line_data + line_len;
		if (fgets(buffer, 1024, fp) == NULL)
		{
			if (line_len == 0)
				line_data[0] = '\0';
			break;
		}
		line_len += strlen(buffer);
	}
	while (!feof(fp) && (line_len == 0 || line_data[line_len-1] != '\n'));

	(*out_ptr) = line_data;
	if (line_len == 0 || line_data[line_len-1] != '\n')
		return EOF;
	line_data[line_len-1] = '\0';
	return 1;
}

/* PERFORMANCE CRITICAL */
/* Binary search a sorted array to find where to insert a data
   element.  If such an element already exists, then the index of the
   existing element is returned.  Otherwise, the index where the new
   element should be inserted is returned.  The `element_exists'
   parameter is set to `true' if an identical element exists, `false'
   otherwise.  The calling function must handle properly initializing
   or modifying the object in the returned slot.  */
unsigned bs_insert_pos(const void *key, const void *array, size_t count,
					   size_t size, misc_cmp_fn_t compare,
					   bool *element_exists)
{
	unsigned sub_pos = 0;
	unsigned sub_len = count;
	unsigned offset = 0;
	/* First, do a binary search for the element until the element is
	   known not to exist in the heap.  Then insert the element at the
	   sorted position.  */
	while (sub_len > 0)
	{
		int cmp_result;
		offset = sub_pos + sub_len / 2;
		cmp_result = compare(key, array + offset * size);
		if (cmp_result < 0)
		{
			sub_len = offset - sub_pos;
		}
		else if (cmp_result > 0)
		{
			sub_len = sub_len + sub_pos - (offset + 1);
			sub_pos = offset + 1;
		}
		else if (cmp_result == 0)
		{
			/* The element was found in the array.  */
		    *element_exists = true;
			return offset;
		}
	}

	/* Check to the left and at the given offset to see whether to
	   insert before or at the given offset.  */
	if (count > 0)
	{
		int low_cmp = 0;
		int cmp_result = compare(key, array + offset * size);
		int high_cmp = 0;
		if (offset > 0)
			low_cmp = compare(key, array + (offset - 1) * size);
		if (offset < count - 1)
			high_cmp = compare(key, array + (offset + 1) * size);
		if (low_cmp < 0 && cmp_result < 0)
			offset--;
		else if (cmp_result > 0 &&
				 (high_cmp < 0 || offset == count - 1))
			offset++;
		/* else if (low_cmp < 0 && cmp_result > 0)
			/\* Do nothing.  *\/;
		else
			fputs("ERROR: Broken algorithm.", stderr); */
	}

	*element_exists = false;
	return offset;
}

/* PERFORMANCE CRITICAL */
/* Similar to bsearch(), but the compare function is guaranteed to be
   called with the key always as the first parameter.  */
void *bsearch_ordered(const void *key, void *array, size_t count,
					  size_t size, misc_cmp_fn_t compare)
{
	unsigned sub_pos = 0;
	unsigned sub_len = count;
	unsigned offset = 0;

	while (sub_len > 0)
	{
		int cmp_result;
		offset = sub_pos + sub_len / 2;
		cmp_result = compare(key, array + offset * size);
		if (cmp_result < 0)
		{
			sub_len = offset - sub_pos;
		}
		else if (cmp_result > 0)
		{
			sub_len = sub_len + sub_pos - (offset + 1);
			sub_pos = offset + 1;
		}
		else if (cmp_result == 0)
		{
			/* The element was found in the array.  */
			return array + offset * size;
		}
	}

	return NULL;
}
