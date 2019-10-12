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

#ifndef STRHEAP_H
#define STRHEAP_H

EA_TYPE(char);
typedef struct StrNode_tag StrNode;
typedef StrNode* StrNode_ptr;
EA_TYPE(StrNode_ptr);
struct StrNode_tag
{
	char_array part; /* not null-terminated */
	unsigned count; /* reference count of this prefix part only */
	StrNode *parent;
	StrNode_ptr_array children; /* children that contain this prefix */
};

void str_init();
void str_dump(const char *filename);
void str_destroy();
StrNode *str_add(char *string);
StrNode *str_ref(StrNode *node);
void str_unref(StrNode *node);
char *str_cstr(StrNode *node);
void str_write(StrNode *node, FILE *fp);
bool str_prefix(StrNode *node, StrNode *prefix);

#endif /* not STRHEAP_H */
