#ifndef GSLICE_H
#define GSLICE_H

#include <glib/gtypes.h>
#include <glib/gmem.h>

#define g_slice_alloc g_malloc
#define g_slice_alloc0 g_malloc
#define g_slice_free1(size, mem) g_free (mem)
#define g_slice_new(type) ((type) g_malloc (sizeof (type)))
#define g_slice_new0(type) ((type) g_slice_alloc0 (sizeof (type)))
#define g_slice_free(type, mem) g_free (mem)
#define g_slice_free_chain(type, mem_chain, next)	\
  {							\
    void * cur_mem = mem_chain;				\
    while (cur_mem->next != NULL) {			\
      void * next_mem;					\
      next_mem = cur_mem->next;				\
      g_slice_free (type, cur_mem);			\
      cur_mem = next_mem;				\
    }							\
  }
#define g_slice_free_chain_with_offset(block_size, mem_chain, next_offset) \
  {									\
    void * cur_mem = mem_chain;						\
    while (cur_mem->next != NULL) {					\
      void * next_mem;							\
      next_mem = &(cur_mem + next_offset);				\
      g_slice_free (type, cur_mem);					\
      cur_mem = next_mem;						\
    }									\
  }

void *g_slice_alloc0 (gsize block_size);

#endif /* not GSLICE_H */
