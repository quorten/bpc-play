/* A weak-typed linked list implementation.

   Ideally, the defined structure will only be a header of the current
   element, and the data of the current element will immediately
   follow the header.  Another possibility is to have the defined
   structure be the first member of another structure. */

#include "bool.h"
#include "null.h"
#include "lnlist.h"

/* Returns a pointer to element "index", where "index" is the index
   number based from the given linked list element.

   If "index" goes beyond the end of the linked list, then NULL will
   be returned.

   If "index" is negative, then the applicable previous element header
   pointer will be returned, or NULL if the negative index goes beyond
   the end of the linked list. */
lnlist *elmt_at(lnlist *reference, int index)
{
	int idx_left;
	lnlist *cur_ref;
	if (reference == NULL)
		return NULL;
	idx_left = index;
	cur_ref = reference;
	if (index >= 0)
	{
		while (idx_left > 0)
		{
			lnlist *next_ref;
			next_ref = cur_ref->next;
			cur_ref = next_ref;
			if (cur_ref == NULL)
				break;
			idx_left--;
		}
	}
	else
	{
		while (idx_left < 0)
		{
			lnlist *prev_ref;
			prev_ref = cur_ref->prev;
			cur_ref = prev_ref;
			if (cur_ref == NULL)
				break;
			idx_left++;
		}
	}
	return cur_ref;
}

/* Returns the first element of a linked list.  */
lnlist *lnlist_begin(lnlist *reference, int *out_offset)
{
	int offset = 0;
	lnlist *begin = reference;
	if (reference == NULL)
	{
		if (out_offset != NULL)
			*out_offset = 0;
		return begin;
	}
	while (reference->prev != NULL)
	{
		begin = reference->prev;
		offset--;
	}
	if (out_offset != NULL)
		*out_offset = offset;
	return begin;
}

/* Returns the last element of a linked list.  */
lnlist *lnlist_end(lnlist *reference, int *out_offset)
{
	int offset = 0;
	lnlist *end = reference;
	if (reference == NULL)
	{
		if (out_offset != NULL)
			*out_offset = 0;
		return end;
	}
	while (reference->next != NULL)
	{
		end = reference->next;
		offset++;
	}
	if (out_offset != NULL)
		*out_offset = offset;
	return end;
}

/* Links (inserts) a linked list element.  If "before" is `true', then
   "elmt" refers to the linked list element that the new element
   should be inserted before.  Otherwise, the new element is inserted
   after the given element.  */
void link_elmt(bool before, lnlist *reference, lnlist *new_elmt)
{
	if (reference == NULL || new_elmt == NULL)
		return;
	if (before)
	{
		new_elmt->next = reference;
		new_elmt->prev = reference->prev;
	}
	else
	{
		new_elmt->next = reference->next;
		new_elmt->prev = reference;
	}
	if (new_elmt->next != NULL) new_elmt->next->prev = new_elmt;
	if (new_elmt->prev != NULL) new_elmt->prev->next = new_elmt;
}

/* Unlinks the specified linked list element.  You will have to
   manually free the data if necessary. */
void unlink_elmt(lnlist *element)
{
	lnlist *prev_elmt;
	lnlist *next_elmt;
	if (element == NULL)
		return;
	prev_elmt = element->prev;
	next_elmt = element->next;
	prev_elmt->next = next_elmt;
	next_elmt->prev = prev_elmt;
}
