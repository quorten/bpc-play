#include <glib/gtypes.h>
#include <string.h>
#include <glib/gmem.h>

void *g_slice_alloc0 (gsize block_size)
{
  void *mem;
  mem = g_malloc (block_size);
  memset (mem, 0, block_size);
  return mem;
}
