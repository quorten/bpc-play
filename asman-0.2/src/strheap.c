/* A simple implementation of a reference-counting storage for C
   strings.

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

#include "bool.h"
#include "xmalloc.h"
#include "misc.h"
#include "exparray.h"
#include "strheap.h"

/* This is the sorted list of reference-counted strings.  */
static StrNode_ptr_array str_heap;

/* Initialize the string reference-counting system.  */
void str_init()
{
	EA_INIT(str_heap, 16);
}

void str_dump(const char *filename)
{
	unsigned i;
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		return;
	for (i = 0; i < str_heap.len; i++)
	{
		fprintf(fp, "%s\n", str_heap.d[i]->d);
		xfree(str_heap.d[i]->d);
		xfree(str_heap.d[i]);
	}
}

/* Shutdown the string reference-counting system.  */
void str_destroy()
{
	EA_DESTROY(str_heap);
}

static int str_add_cmp_func(const void *key, const void *aentry)
{
	const char *string = (const char*)key;
	const StrNode_ptr *node = (const StrNode_ptr*)aentry;
	return strcmp(string, (*node)->d);
}

/* Add a reference-counted string to the string heap.  After the
   string is added, memory ownership of the string passes on to the
   string heap.  A pointer to the reference counting segment is
   returned.  As always with any kind of garbage collector, this
   reference counter does not make your code invulnerable to the
   problem of unused memory still being allocated.  */
StrNode *str_add(char *string)
{
	bool element_exists;
	unsigned ins_pos = bs_insert_pos(string, str_heap.d, str_heap.len,
									 sizeof(StrNode_ptr),
									 str_add_cmp_func, &element_exists);
	if (element_exists)
	{
		/* Do not insert the string.  Increase the reference count
		   instead.  */
		str_heap.d[ins_pos]->count++;
		if (string != str_heap.d[ins_pos]->d)
			xfree(string);
		return str_heap.d[ins_pos];
	}

	/* Insert the element.  */
	EA_INS(str_heap, ins_pos);
	str_heap.d[ins_pos] = (StrNode*)xmalloc(sizeof(StrNode));
	str_heap.d[ins_pos]->d = string;
	str_heap.d[ins_pos]->count = 1;
	return str_heap.d[ins_pos];
}

/* Increase the reference count of a string.  */
void str_ref(StrNode *obj)
{
	obj->count++;
}

/* Decrease the reference count of a string.  The string is freed and
   the tracking segment is removed if the reference count reaches
   zero.  */
void str_unref(StrNode *obj)
{
	obj->count--;
	if (obj->count == 0)
	{
		unsigned i;
		xfree(obj->d);
		/* Remove the tracking segment from the heap.  */
		for (i = 0; i < str_heap.len; i++)
		{
			if (str_heap.d[i] == obj)
			{
				EA_REMOVE(str_heap, i);
				xfree(obj);
				break;
			}
		}
	}
}

char *str_cstr(StrNode *node)
{
	return xstrdup(node->d);
}

/* Check if the given string contains the given prefix.  */
bool str_prefix(StrNode *node, StrNode *prefix)
{
	if (node == prefix)
		return true;
	if (strncmp(node->d, prefix->d, strlen(prefix->d)) == 0)
		return true;
	return false;
}
