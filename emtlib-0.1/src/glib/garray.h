/* GLib GArray compatibility header.  */

#ifndef GARRAY_H
#define GARRAY_H

#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib/gmem.h>

#define EA_GARRAY_REALLOC
#define ea_malloc g_malloc
#define ea_realloc g_realloc
#define ea_free g_free
#define _GRealWrap _GRealArray
#include <exparray.h>

struct _GArray
{
  gchar *data;
  guint len;
};
typedef struct _GArray GArray;

struct _GPtrArray
{
  gpointer *pdata;
  guint len;
};
typedef struct _GPtrArray GPtrArray;

struct _GByteArray
{
  guint8 *data;
  guint len;
};
typedef struct _GByteArray GByteArray;

EA_TYPE(gchar);

#define g_array_append_val(a, v) \
  if (a != NULL) EA_APPEND_MULT(gchar, *((gchar_array *) a), v, 1)
#define g_array_prepend_val(a, v) \
  if (a != NULL) EA_PREPEND_MULT(gchar, *((gchar_array *) a), v, 1)
#define g_array_insert_val(a, i, v) \
  if (a != NULL) EA_INSERT(gchar, *((gchar_array *) a), i, v)
#define g_array_index(a, t, i) \
  (((t*) (a)->data)[i])

GArray *g_array_new(gboolean zero_terminated, gboolean clear_,
		    guint element_size);
GArray *g_array_sized_new(gboolean zero_terminated, gboolean clear_,
			  guint element_size, guint reserved_size);
gchar *g_array_free(GArray *array, gboolean free_segment);
void g_array_sort(GArray *array, GCompareFunc compare_func);
void g_array_sort_with_data(GArray *array, GCompareDataFunc compare_func,
			    gpointer user_data);

GArray *g_array_append_vals(GArray *array, gconstpointer data, guint len);
GArray *g_array_prepend_vals(GArray *array, gconstpointer data, guint len);
GArray *g_array_insert_vals(GArray *array, guint index_,
			    gconstpointer data, guint len);
GArray *g_array_remove_index(GArray *array, guint index_);
GArray *g_array_remove_index_fast(GArray *array, guint index_);
GArray *g_array_remove_range(GArray *array, guint index_, guint length);
GArray *g_array_set_size(GArray *array, guint length);

/* Pointer Arrays */

#define g_ptr_array_index(array, index_) \
  (array->pdata[index_])

void g_ptr_array_foreach (GPtrArray *array, GFunc func, gpointer user_data);
GPtrArray *g_ptr_array_new ();
GPtrArray *g_ptr_array_sized_new (guint reserved_size);
void g_ptr_array_add (GPtrArray *array, gpointer data);
gboolean g_ptr_array_remove (GPtrArray *array, gpointer data);
gpointer g_ptr_remove_index (GPtrArray *array, guint index);
gboolean g_ptr_array_remove_fast (GPtrArray *array, gpointer data);
gpointer g_ptr_remove_index_fast (GPtrArray *array, guint index);
void g_ptr_array_remove_range (GPtrArray *array, guint index_, guint length);
void g_ptr_array_sort (GPtrArray *array, GCompareFunc compare_func);
void g_ptr_array_sort_with_data (GPtrArray *array,
		 GCompareDataFunc compare_func, gpointer user_data);
void g_ptr_array_set_size (GPtrArray *array, gint length);
gpointer g_ptr_array_free (GPtrArray *array, gboolean free_segment);

/* Byte Arrays */

GByteArray *g_byte_array_new ();
GByteArray *g_byte_array_sized_new (guint reserved_size);
GByteArray *g_byte_array_append (GByteArray *array, const guint8 *data,
				 guint len);
GByteArray *g_byte_array_prepend (GByteArray *array, const guint8 *data,
				  guint len);
GByteArray *g_byte_array_remove_index (GByteArray *array, guint index_);
GByteArray *g_byte_array_remove_index_fast (GByteArray *array, guint index_);
GByteArray *g_byte_array_remove_range (GByteArray *array, guint index_,
				       guint length);
void g_byte_array_sort (GByteArray *array, GCompareFunc compare_func);
void g_byte_array_sort_with_data (GByteArray *array,
		  GCompareDataFunc compare_func, gpointer user_data);
GByteArray* g_byte_array_set_size (GByteArray *array, guint length);
guint8* g_byte_array_free (GByteArray *array, gboolean free_segment);

#endif /* not GARRAY_H */
