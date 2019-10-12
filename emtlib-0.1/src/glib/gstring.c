/* A lightweight replacement for GString.

   This implementation is basically a thin wrapper for exparray.  */

#include <string.h>
#include <stdarg.h>
#include <glib/gmacros.h>
#include <glib/gtypes.h>
#include <glib/gmem.h>
#include <glib/gstring.h>

#define EA_GARRAY_REALLOC
#define ea_malloc g_malloc
#define ea_realloc g_realloc
#define ea_free g_free
#include <exparray.h>

#define EA_GSTRING_REALLOC

struct _GStringReal
{
  gchar *d;
  gsize len;
  gsize ea_len_alloc;
  unsigned tysize;
  gboolean zero_term;
  gboolean clear;
};

/* Wrapper structure for exparray */
struct _GRealWrap
{
  gchar *data;
  gsize len;
  gsize ea_len_alloc;
  unsigned tysize;
  gboolean zero_term;
  gboolean clear;
};

EA_TYPE (gchar);

GString *g_string_new_len (const gchar *init, gssize len)
{
  struct _GStringReal *new_str;
  new_str = (struct _GStringReal *)
    g_malloc (sizeof (struct _GStringReal));
  EA_INIT (gchar, *new_str, 1);
  new_str->d[0] = '\0';
  new_str->zero_term = TRUE;
  if (init != NULL)
    {
      EA_SET_SIZE (gchar, *new_str, len);
      strncpy (new_str->d, init, len);
    }
  return (GString *) new_str;
}

GString *g_string_sized_new (gsize alloc_size)
{
  GString *new_str;
  new_str = g_string_new_len ("", alloc_size);
  new_str->len = 0;
  return new_str;
}

GString *g_string_assign (GString *string, const gchar *value)
{
  EA_SET_SIZE (gchar, *((struct _GStringReal *) string), strlen (value));
  strcpy (string->str, value);
  return string;
}

gchar *g_string_free (GString *string, gboolean free_segment)
{
  gchar *data;
  if (string == NULL) return NULL;
  if (free_segment)
    {
      data = NULL;
      ea_free (string->str);
    }
  else
    data = string->str;
  g_free (string);
  return data;
}

gboolean g_string_equal (const GString *str1, const GString *str2)
{
  if (str1 == NULL || str2 == NULL) return FALSE;
  if (!memcmp (str1, str2, (str1->len > str2->len) ? str1->len : str2->len))
    return TRUE;
  return FALSE;
}

/* Thin Wrappers */

GString *g_string_new (const gchar *init)
{ return g_string_new_len (init, strlen (init)); }

GString *g_string_append (GString *string, const gchar *value)
{ return g_string_append_len (string, value, strlen (value)); }

GString *g_string_append_c (GString *string, gchar c)
{ if (string == NULL) return NULL;
  EA_APPEND (gchar, *((struct _GStringReal *) string), c);
  return string; }

GString *g_string_append_len (GString *string, const gchar *value,
			      gssize len)
{ if (string == NULL) return NULL;
  EA_APPEND_MULT (gchar, *((struct _GStringReal *) string),
		  value, len);
  return string; }

GString *g_string_prepend (GString *string, const gchar *value)
{ return g_string_prepend_len (string, value, strlen (value)); }

GString *g_string_prepend_c (GString *string, gchar c)
{ if (string == NULL) return NULL;
  EA_PREPEND (gchar, *((struct _GStringReal *) string), c);
  return string; }

GString *g_string_prepend_len (GString *string, const gchar *value,
			       gssize len)
{ if (string == NULL) return NULL;
  EA_PREPEND_MULT (gchar, *((struct _GStringReal *) string),
		   value, len);
  return string; }

GString *g_string_insert (GString *string, gssize pos, const gchar *value)
{ return g_string_insert_len (string, pos, value, strlen (value)); }

GString *g_string_insert_c (GString *string, gssize pos, gchar c)
{ if (string == NULL) return NULL;
  EA_INSERT (gchar, *((struct _GStringReal *) string), pos, c);
  return string; }

GString *g_string_insert_len (GString *string, gssize pos,
			      const gchar *value, gssize len)
{ if (string == NULL) return NULL;
  EA_INSERT_MULT (gchar, *((struct _GStringReal *) string), pos,
		  value, len);
  return string; }

GString *g_string_erase (GString *string, gssize pos, gssize len)
{ if (string == NULL) return NULL;
  EA_REMOVE_RANGE (gchar, *((struct _GStringReal *) string),
		   pos, len);
  return string; }

GString *g_string_truncate (GString *string, gsize len)
{ return g_string_set_size (string, len); }

GString *g_string_set_size (GString *string, gsize len)
{ if (string == NULL) return NULL;
  EA_SET_SIZE (gchar, *((struct _GStringReal *) string), len);
  return string; }
