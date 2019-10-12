#ifndef GMEM_H
#define GMEM_H

#include <stdlib.h>
#include <xmalloc.h>

gpointer g_malloc0 (gulong n_bytes);
gpointer g_try_malloc0 (gulong n_bytes);

#define g_malloc xmalloc
#define g_realloc xrealloc
#define g_try_malloc malloc
#define g_try_realloc realloc
#define g_free xfree
#define g_alloca alloca
#define g_newa(struct_type, n_structs) \
  alloca (sizeof (struct_type) * n_structs);
#define g_memmove memmove
#define g_mem_is_system_malloc() TRUE
#define g_mem_profile() ; /* Empty */

#endif /* not GMEM_H */
