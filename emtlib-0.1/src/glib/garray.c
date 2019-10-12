/* A lightweight replacement for GLib's GArray.

   Note that this implementation is generally much slower and less
   efficient than GLib's GArray implementation.  You should only use
   this mechanism if you have a really good reason not to use GLib's
   GArray.  Really, I no longer see any reason to avoid GLib.  You
   should be just fine with modifying GLib to use a subset of it, if
   that is what you really need to do.  */

#include <stdlib.h>
#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib/gmem.h>
#include <glib/gqsort.h>
#include <glib/garray.h>

struct _GRealArray
{
  gchar *data;
  guint len;
  guint tysize;
  guint ea_len_alloc;
  gboolean clear;
  gboolean zero_term;
};

GArray *g_array_new(gboolean zero_terminated, gboolean clear_,
		    guint element_size)
{
  return g_array_sized_new(zero_terminated, clear_, element_size, 1);
}

GArray *g_array_sized_new(gboolean zero_terminated, gboolean clear_,
			   guint element_size, guint reserved_size)
{
  struct _GRealArray *new_array;
  new_array = (struct _GRealArray *) g_malloc(sizeof(struct _GRealArray));
  if (new_array == NULL)
    abort();
  new_array->len = 0;
  new_array->tysize = element_size;
  new_array->clear = clear_;
  new_array->zero_term = zero_terminated;
  new_array->ea_len_alloc = reserved_size;
  if (new_array->zero_term)
    new_array->ea_len_alloc++;
  new_array->data = (gchar *) g_malloc(new_array->tysize *
				       new_array->ea_len_alloc);
  if (new_array->data == NULL)
    abort();
  if (new_array->clear)
    memset(new_array->data, 0, new_array->tysize * new_array->ea_len_alloc);
  else if (new_array->zero_term)
    memset(new_array->data, 0, new_array->tysize * 1);
  return (GArray *)new_array;
}

gchar *g_array_free(GArray *array, gboolean free_segment)
{
  gchar *segment = NULL;
  if (array == NULL) return NULL;
  if (!free_segment)
    segment = array->data;
  else
    g_free(array->data);
  g_free(array);
  return segment;
}

void g_array_sort(GArray *array, GCompareFunc compare_func)
{
  struct _GRealArray *rarray = (struct _GRealArray *) array;
  if (array == NULL) return;
  if (compare_func == NULL) return;
  qsort(rarray->data, rarray->len, rarray->tysize, compare_func);
}

void g_array_sort_with_data(GArray *array, GCompareDataFunc compare_func,
			    gpointer user_data)
{
  struct _GRealArray *rarray = (struct _GRealArray *) array;
  if (array == NULL) return;
  if (compare_func == NULL) return;
  g_qsort_with_data(rarray->data, rarray->len, rarray->tysize, compare_func,
		    user_data);
}

/* Thin Wrappers */

GArray *g_array_append_vals(GArray *array, gconstpointer data, guint len)
{ if (array != NULL)
    EA_APPEND_MULT(gchar, *((gchar_array *) array), data, len);
  return array; }
GArray *g_array_prepend_vals(GArray *array, gconstpointer data, guint len)
{ if (array != NULL)
    EA_PREPEND_MULT(gchar, *((gchar_array *) array), data, len);
  return array; }
GArray *g_array_insert_vals(GArray *array, guint index_,
			    gconstpointer data, guint len)
{ if (array != NULL)
    EA_INSERT_MULT(gchar, *((gchar_array *) array), index_, data, len);
  return array; }
GArray *g_array_remove_index(GArray *array, guint index_)
{ if (array != NULL) EA_REMOVE(gchar, *((gchar_array *) array), index_);
  return array; }
GArray *g_array_remove_index_fast(GArray *array, guint index_)
{ if (array != NULL)
    EA_REMOVE_FAST(gchar, *((gchar_array *) array), index_);
  return array; }
GArray *g_array_remove_range(GArray *array, guint index_, guint length)
{ if (array != NULL)
    EA_REMOVE_RANGE(gchar, *((gchar_array *) array), index_, length);
  return array; }
GArray *g_array_set_size(GArray *array, guint length)
{ if (array != NULL) EA_SET_SIZE(gchar, *((gchar_array *) array), length);
  return array; }

/* Pointer Arrays */

static gboolean
g_ptr_array_remove_real (GPtrArray *array, gpointer data, gboolean fast)
{
  guint pos = 0;
  gboolean found_ptr = FALSE;
  if (array == NULL)
    return found_ptr;
  while (pos < array->len)
    {
      if (array->pdata[pos] == data)
	{
	  found_ptr = TRUE;
	  break;
	}
    }
  if (found_ptr)
    {
      if (fast)
	g_array_remove_index_fast ((GArray *) array, pos);
      else
	g_array_remove_index ((GArray *) array, pos);
    }
  return found_ptr;
}

static gpointer
g_ptr_array_remove_index_real (GPtrArray *array, guint index, gboolean fast)
{
  gpointer old_ptr;
  if (array == NULL)
    return NULL;
  old_ptr = array->pdata[index];
  if (fast)
    g_array_remove_index_fast ((GArray *) array, index);
  else
    g_array_remove_index ((GArray *) array, index);
  return old_ptr;
}

void g_ptr_array_foreach (GPtrArray *array, GFunc func, gpointer user_data)
{
  guint pos;
  if (array == NULL || func == NULL)
    return;
  for (pos = 0; pos < array->len; pos++)
    func (array->pdata[pos], user_data);
}

GPtrArray *g_ptr_array_new ()
{ return g_ptr_array_sized_new (1); }
GPtrArray *g_ptr_array_sized_new (guint reserved_size)
{ return (GPtrArray *) g_array_sized_new (FALSE, FALSE,
				  sizeof (gpointer), reserved_size); }
void g_ptr_array_add (GPtrArray *array, gpointer data)
{ g_array_append_val ((GArray *) array, data); }
gboolean g_ptr_array_remove (GPtrArray *array, gpointer data)
{ return g_ptr_array_remove_real (array, data, FALSE); }
gpointer g_ptr_remove_index (GPtrArray *array, guint index)
{ return g_ptr_array_remove_index_real (array, index, FALSE); }
gboolean g_ptr_array_remove_fast (GPtrArray *array, gpointer data)
{ return g_ptr_array_remove_real (array, data, TRUE); }
gpointer g_ptr_remove_index_fast (GPtrArray *array, guint index)
{ return g_ptr_array_remove_index_real (array, index, TRUE); }
void g_ptr_array_remove_range (GPtrArray *array, guint index_, guint length)
{ g_array_remove_range ((GArray *) array, index_, length); }
void g_ptr_array_sort (GPtrArray *array, GCompareFunc compare_func)
{ g_array_sort ((GArray *) array, compare_func); }
void g_ptr_array_sort_with_data (GPtrArray *array,
			 GCompareDataFunc compare_func, gpointer user_data)
{ g_array_sort_with_data ((GArray *) array, compare_func, user_data); }
void g_ptr_array_set_size (GPtrArray *array, gint length)
{ g_array_set_size ((GArray *) array, length); }
gpointer g_ptr_array_free (GPtrArray *array, gboolean free_segment)
{ return (gpointer) g_array_free ((GArray *) array, free_segment); }

/* Byte Arrays */

GByteArray *g_byte_array_new ()
{ return g_byte_array_sized_new (1); }
GByteArray *g_byte_array_sized_new (guint reserved_size)
{ return (GByteArray *) g_array_new (FALSE, FALSE, sizeof (guint8)); }
GByteArray *g_byte_array_append (GByteArray *array, const guint8 *data,
				 guint len)
{ g_array_append_vals ((GArray *) array, data, len);
  return array; }
GByteArray *g_byte_array_prepend (GByteArray *array, const guint8 *data,
				  guint len)
{ g_array_prepend_vals ((GArray *) array, data, len);
  return array; }
GByteArray *g_byte_array_remove_index (GByteArray *array, guint index_)
{ g_array_remove_index ((GArray *) array, index_);
  return array; }
GByteArray *g_byte_array_remove_index_fast (GByteArray *array, guint index_)
{ g_array_remove_index_fast ((GArray *) array, index_);
  return array; }
GByteArray *g_byte_array_remove_range (GByteArray *array, guint index_,
				       guint length)
{ g_array_remove_range ((GArray *) array, index_, length);
  return array; }
void g_byte_array_sort (GByteArray *array, GCompareFunc compare_func)
{ g_array_sort ((GArray *) array, compare_func); }
void g_byte_array_sort_with_data (GByteArray *array,
		  GCompareDataFunc compare_func, gpointer user_data)
{ g_array_sort_with_data ((GArray *) array, compare_func, user_data); }
GByteArray *g_byte_array_set_size (GByteArray *array, guint length)
{ g_array_set_size ((GArray *) array, length);
  return array; }
guint8 *g_byte_array_free (GByteArray *array, gboolean free_segment)
{ return (guint8 *) g_array_free ((GArray *) array, free_segment); }
