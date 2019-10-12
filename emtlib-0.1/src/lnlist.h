/* A weak-typed linked list implementation.

   Ideally, the defined structure will only be a header of the current
   element, and the data of the current element will immediately
   follow the header.  Another possibility is to have the defined
   structure be the first member of another structure. */

#ifndef LNLIST_H
#define LNLIST_H

/* Note that we use Ghostscript/Python OOP notation to decrease the
   likelihood that users will manually type in the fields.  Users can
   choose to use either Ghostscript/Python OOP notation or GObject OOP
   notation in their programs this way.  */
#define slnlist_common(type) \
	type *next
#define lnlist_common(type) \
	slnlist_common(type); \
	type *prev
typedef struct lnlist_tag lnlist;
struct lnlist_tag
{
	lnlist_common(lnlist);
};

lnlist *elmt_at(lnlist *reference, int index);
lnlist *lnlist_begin(lnlist *reference, int *out_offset);
lnlist *lnlist_end(lnlist *reference, int *out_offset);
void link_elmt(bool before, lnlist *reference, lnlist *new_elmt);
void unlink_elmt(lnlist *element);

#endif /* LNLIST_H */
