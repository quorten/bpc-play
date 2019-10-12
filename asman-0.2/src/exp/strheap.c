/* A hierarchical, reference-counting, string storage mechanism.

Copyright (C) 2013 Andrew Makousky

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
#include <stddef.h>
#include "bool.h"
#include "xmalloc.h"
#include "exparray.h"
#include "strheap.h"

typedef struct lnlist_tag lnlist;
struct lnlist_tag
{
	lnlist *next;
	StrNode *node;
};

/* This is the root of the reference-counted string tree.  */
static StrNode str_heap;

/* Private Declarations */
void str_dump_node(StrNode *node, FILE *fp, unsigned indent_level);
void str_destroy_node(StrNode *node);
int strpcmp(char *s1, char *s2, unsigned maxlen);
StrNode *str_search(StrNode *node, char *string);
StrNode *str_fork(StrNode *node, char *string);

/* Initialize the string reference-counting system.  */
void str_init()
{
	str_heap.part.d = NULL;
	str_heap.part.len = 0;
	str_heap.count = 1;
	str_heap.parent = NULL;
	EA_INIT(StrNode_ptr, str_heap.children, 4);
}

void str_dump_node(StrNode *node, FILE *fp, unsigned indent_level)
{
	unsigned i;
	for (i = 0; i < node->part.len; i++)
	{
		switch (node->part.d[i])
		{
		case '\n':
			putc('\\', fp); putc('n', fp);
			indent_level += 2;
			break;
		case '\\':
			putc('\\', fp); putc('\\', fp);
			indent_level += 2;
			break;
		default:
			putc(node->part.d[i], fp);
			indent_level++;
			break;
		}
	}
	if (i > 0)
		putc('\n', fp);

	for (i = 0; i < node->children.len; i++)
	{
		unsigned j;
		for (j = 0; j < indent_level; j++)
			putc(' ', fp);
		str_dump_node(node->children.d[i], fp, indent_level);
	}
}

/* Dump the contents of the string heap to a file.  */
void str_dump(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp == NULL)
		return;
	str_dump_node(&str_heap, fp, 0);
	fclose(fp);
}

void str_destroy_node(StrNode *node)
{
	unsigned i;
	EA_DESTROY(node->part);
	for (i = 0; i < node->children.len; i++)
		str_destroy_node(node->children.d[i]);
	EA_DESTROY(node->children);
}

/* Shutdown the string reference-counting system.  */
void str_destroy()
{
	str_destroy_node(&str_heap);
}

/* Compare two strings up to `maxlen' characters and return 0 if the
   strings are equal, 1 if s1 > s2, -1 if s1 < s2, -2 if s1 is the
   prefix of s2, and 2 if s2 is the prefix of s1.  */
int strpcmp(char *s1, char *s2, unsigned maxlen)
{
}

/* Search for a string in a tree node's array and return an existing
   node that corresponds to the given string, or create and return a
   new one.  */
StrNode *str_search(StrNode *node, char *string)
{
	unsigned sub_pos = 0;
	unsigned sub_len = node->children.len;
	unsigned offset = 0;
	/* First compare the first characters to find where to insert the
	   string in the array.  */
	/* Basically, do a binary search for the first character of the
	   string until the string is known not to exist in the array.
	   Then insert the string at the sorted position.  */
	while (sub_len > 0)
	{
		const size_t waste_size = sizeof(void*) * 5;
		size_t save_size;
		int cmp_result;
		offset = sub_pos + sub_len / 2;
		cmp_result = strncmp(string, node->children.d[offset]->part.d,
							 node->children.d[offset]->part.len);
		save_size *= node->children.d[offset]->count + 1;
		if (cmp_result < 0)
			sub_len = offset - sub_pos;
		else if (cmp_result > 0)
		{
			sub_len = sub_len + sub_pos - (offset + 1);
			sub_pos = offset + 1;
		}
		else if (cmp_result == 0 && save_size >= waste_size)
		{
			/* Traverse this node to check how much of the string
			   already exists.  */
			return str_fork(node->children.d[offset], string);
		}
	}

	/* Check to the left and at the given offset to see whether to
	   insert before or at the given offset.  */
	if (node->children.len > 0)
	{
		int low_cmp = 0;
		int cmp_result =
			strncmp(string, node->children.d[offset]->part.d,
					node->children.d[offset]->part.len);
		int high_cmp = 0;
		if (offset > 0)
			low_cmp = strncmp(string, node->children.d[offset-1]->part.d,
							  node->children.d[offset-1]->part.len);
		if (offset < node->children.len - 1)
			high_cmp = strncmp(string, node->children.d[offset+1]->part.d,
							   node->children.d[offset+1]->part.len);
		if (low_cmp < 0 && cmp_result < 0)
			offset--;
		else if (cmp_result > 0 &&
				 (high_cmp < 0 || offset == node->children.len - 1))
			offset++;
		/* else if (low_cmp < 0 && cmp_result > 0)
			/\* Do nothing.  *\/;
		else
			fputs("ERROR: Broken algorithm.", stderr); */
	}

	{ /* Insert the element.  */
		StrNode *new_node = (StrNode*)xmalloc(sizeof(StrNode));
		unsigned prefix_len = strlen(string);
		EA_INIT(char, new_node->part, 16);
		EA_SET_SIZE(new_node->part, prefix_len);
		memcpy(new_node->part.d, string, prefix_len);
		new_node->part.d[prefix_len] = '\0';
		new_node->count = 1;
		new_node->parent = node;
		EA_INIT(StrNode_ptr, new_node->children, 4);
		EA_INSERT(node->children, offset, new_node);
	}
	return node->children.d[offset];
}

/* Walk the prefix string in a given node and split/append to it to
   add the given string, or increase the reference count if the new
   string is equal to the prefix.  */
StrNode *str_fork(StrNode *node, char *string)
{
	/* Walk down the prefix and the string.  */
	char *pfxpos = node->part.d;
	char *pfxend = node->part.d + node->part.len;
	char *strpos = string;
	while (pfxpos != pfxend && *pfxpos == *strpos)
	{ pfxpos++; strpos++; }

	/* Check if the new string ends before the prefix.
	   Example: prefix  = "abcdef"
	            new_str = "abc" */
	if (pfxpos != pfxend && *strpos == '\0')
	{
		/* Split the prefix at the ending point and initialize the new
		   intermediate node.  */
		StrNode *parent = node->parent;
		StrNode *new_node = (StrNode*)xmalloc(sizeof(StrNode));
		unsigned new_pfx_len = pfxpos - node->part.d;
		unsigned break_len = node->part.len - new_pfx_len;
		unsigned i;

		EA_INIT(char, new_node->part, 4);
		EA_APPEND_MULT(new_node->part, node->part.d, new_pfx_len);
		EA_REMOVE_MULT(node->part, 0, new_pfx_len);
		new_node->count = 1;
		new_node->parent = parent;
		node->parent = new_node;
		for (i = 0; i < parent->children.len; i++)
		{
			if (parent->children.d[i] == node)
			{
				parent->children.d[i] = new_node;
				break;
			}
		}
		EA_INIT(StrNode_ptr, new_node->children, 4);
		EA_APPEND(new_node->children, node);
		return new_node;
	}

	/* Check if there is a point where the new string is inequal to
	   the prefix.
	   Example: prefix  = "abcdef"
	            new_str = "abzx" */
	else if (pfxpos != pfxend)
	{
		/* Split both strings at the common prefix and add the two
		   parts separately.  */
		StrNode *parent = node->parent;
		StrNode *new_pfx_node = (StrNode*)xmalloc(sizeof(StrNode));
		unsigned comm_pfx_len = pfxpos - node->part.d;
		unsigned break_len = node->part.len - comm_pfx_len;
		unsigned i;

		/* Initialize the new prefix node and modify existing nodes to
		   put it in the tree.  */
		EA_INIT(char, new_pfx_node->part, 4);
		EA_APPEND_MULT(new_pfx_node->part, node->part.d, comm_pfx_len);
		EA_REMOVE_MULT(node->part, 0, comm_pfx_len);
		new_pfx_node->count = 0;
		new_pfx_node->parent = parent;
		node->parent = new_pfx_node;
		for (i = 0; i < parent->children.len; i++)
		{
			if (parent->children.d[i] == node)
			{
				parent->children.d[i] = new_pfx_node;
				break;
			}
		}
		EA_INIT(StrNode_ptr, new_pfx_node->children, 4);
		EA_APPEND(new_pfx_node->children, node);

		/* Next add the node for the new string.  */
		return str_search(new_pfx_node, strpos);
	}

	/* Check if the new string is equal to the prefix.  */
	if (pfxpos == pfxend && *strpos == '\0')
	{
		/* Easy way out.  */
		node->count++;
		return node;
	}

	/* Check if the prefix is entirely a prefix of the new string.
	   Example: prefix  = "abc"
	            new_str = "abcdef" */
	else if (pfxpos == pfxend)
	{
		/* Break the new string at the prefix ending point and add the
		   leftover of the new string.  */
		return str_search(node, strpos);
	}
}

/* Add a reference-counted string to the string heap.  After the
   string is added, memory ownership of the string passes on to the
   string heap.  A pointer to the reference counting segment is
   returned.  As always with any kind of garbage collector, this
   reference counter does not make your code invulnerable to the
   problem of unused memory still being allocated.  */
StrNode *str_add(char *string)
{
	StrNode *node = str_search(&str_heap, string);
	xfree(string);
	return node;
}

/* Increase the reference count of a string.  */
StrNode *str_ref(StrNode *node)
{
	node->count++;
	return node;
}

/* Decrease the reference count of a string.  The string is freed and
   the tracking segment is removed if the reference count reaches zero
   and there are no children of the given prefix.  */
void str_unref(StrNode *node)
{
	if (node->count > 0)
		node->count--;
	if (node->count == 0 && node->children.len == 0)
	{
		StrNode *parent = node->parent;
		unsigned i;
		for (i = 0; i < parent->children.len; i++)
		{
			if (parent->children.d[i] == node)
				EA_REMOVE(parent->children, i);
		}
		EA_DESTROY(node->part);
		EA_DESTROY(node->children);
		xfree(node);
		if (parent->count == 0 && parent->children.len == 0 &&
			parent != &str_heap)
			return str_unref(parent);
	}
}

/* Given a string tree node, walk backwards and generate a C string
   for the given node.  The returned string is dynamically allocated
   and must be freed.  */
char *str_cstr(StrNode *node)
{
	lnlist *path = NULL;
	char *cstr;
	unsigned cstr_len = 0;
	char *cstr_pos;
	/* Walk backwards on the hierarchy, building a list of path parts
	   in forwards order.  */
	while (node->parent != NULL)
	{
		lnlist *new_part;
		new_part = (lnlist*)xmalloc(sizeof(lnlist));
		new_part->node = node;
		new_part->next = path;
		path = new_part;
		cstr_len += node->part.len;
		node = node->parent;
	}
	/* Now build the C string.  */
	cstr = (char*)xmalloc(cstr_len + 1);
	cstr_pos = cstr;
	while (path != NULL)
	{
		lnlist *next_part = path->next;
		strncpy(cstr_pos, path->node->part.d, path->node->part.len);
		cstr_pos += path->node->part.len;
		xfree(path);
		path = next_part;
	}
	*cstr_pos = '\0';
	return cstr;
}

/* Write out the given string to the given file.  */
void str_write(StrNode *node, FILE *fp)
{
	lnlist *path = NULL;
	/* Walk backwards on the hierarchy, building a list of path parts
	   in forwards order.  */
	while (node->parent != NULL)
	{
		lnlist *new_part;
		new_part = (lnlist*)xmalloc(sizeof(lnlist));
		new_part->node = node;
		new_part->next = path;
		path = new_part;
		node = node->parent;
	}
	/* Now write the string.  */
	while (path != NULL)
	{
		lnlist *next_part = path->next;
		char_array *cur_part = &path->node->part;
		unsigned i;
		for (i = 0; i < cur_part->len; i++)
			putc(cur_part->d[i], fp);
		xfree(path);
		path = next_part;
	}
}

/* Check if the given string contains the given prefix.  */
bool str_prefix(StrNode *node, StrNode *prefix)
{
	if (node->parent == NULL)
		return false;
	if (node->parent == prefix)
		return true;
	return str_precmp(node->parent, prefix);
}
