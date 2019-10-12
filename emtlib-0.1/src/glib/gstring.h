/* A lightweight replacement for GString.  */
#ifndef GSTRING_H
#define GSTRING_H

struct _GString
{
  gchar  *str;
  gsize len;
  gsize allocated_len;
};

typedef struct _GString GString;

GString *g_string_new (const gchar *init);
GString *g_string_new_len (const gchar *init, gssize len);
GString *g_string_sized_new (gsize alloc_size);
GString *g_string_assign (GString *string, const gchar *value);
/* void g_string_printf (GString *string, const gchar *format, ...); */
/* void g_string_append_printf (GString *string, const gchar *format, ...); */
gchar *g_string_free (GString *string, gboolean free_segment);

GString *g_string_append (GString *string, const gchar *value);
GString *g_string_append_c (GString *string, gchar c);
/* GString *g_string_append_unichar (GString *string, gunichar wc); */
GString *g_string_append_len (GString *string, const gchar *value,
			      gssize len);
GString *g_string_prepend (GString *string, const gchar *value);
GString *g_string_prepend_c (GString *string, gchar c);
/* GString *g_string_prepend_unichar (GString *string, gunichar wc); */
GString *g_string_prepend_len (GString *string, const gchar *value,
			       gssize len);
GString *g_string_insert (GString *string, gssize pos, const gchar *value);
GString *g_string_insert_c (GString *string, gssize pos, gchar c);
/* GString *g_insert_unichar (GString *string, gssize pos, gunichar wc); */
GString *g_string_insert_len (GString *string, gssize pos,
			      const gchar *value, gssize len);
GString *g_string_erase (GString *string, gssize pos, gssize len);
GString *g_string_truncate (GString *string, gsize len);
GString *g_string_set_size (GString *string, gsize len);
/* guint g_string_hash (const GString *str); */
gboolean g_string_equal (const GString *str1, const GString *str2);

#endif /* not GSTRING_H */
