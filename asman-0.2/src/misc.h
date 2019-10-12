/* Miscellaneous, but important, common functions.

Copyright (C) 2013 Andrew Makousky

See the file "COPYING" in the top level directory for details.

*/

#ifndef MISC_H
#define MISC_H

/* Macros to assist selecting a subset of a C string by temporarily
   splicing it in place.  */
#define STR_BEGIN_SPLICE(end_ptr) \
	{ \
		char *char_pos; \
		char last_char; \
		char_pos = end_ptr; \
		last_char = *char_pos; \
		*char_pos = '\0';
#define STR_END_SPLICE() \
		*char_pos = last_char; \
	}

typedef int misc_cmp_fn_t (const void *key, const void *aentry);

int exp_getline(FILE *fp, char **out_ptr);
unsigned bs_insert_pos(const void *key, const void *array, size_t count,
					   size_t size, misc_cmp_fn_t compare,
					   bool *element_exists);
void *bsearch_ordered(const void *key, void *array, size_t count,
					  size_t size, misc_cmp_fn_t compare);

#endif /* not MISC_H */
