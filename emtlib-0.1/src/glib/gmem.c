/* Thin wrappers for `gmem'.  */

#include <stdlib.h>
#include <string.h>
#include <xmalloc.h>
#include <glib/gtypes.h>
#include <glib/gmem.h>

gpointer g_malloc0 (gulong n_bytes)
{
  gpointer mem;
  mem = xmalloc (n_bytes);
  memset (mem, 0, n_bytes);
  return mem;
}

gpointer g_try_malloc0 (gulong n_bytes)
{
  gpointer mem;
  mem = malloc (n_bytes);
  if (mem != NULL)
    memset (mem, 0, n_bytes);
  return mem;
}
