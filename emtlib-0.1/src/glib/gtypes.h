#ifndef GTYPES_H
#define GTYPES_H

typedef unsigned char guint8;
typedef char gchar;
typedef unsigned int guint;
typedef int gint;
typedef unsigned long gulong;
typedef int gboolean;
typedef void * gpointer;
typedef const void * gconstpointer;
typedef gint (*GCompareFunc) (gconstpointer a, gconstpointer b);
typedef gint (*GCompareDataFunc) (gconstpointer a, gconstpointer b,
				  gpointer user_data);
typedef void (*GFunc) (gpointer data, gpointer user_data);

typedef unsigned int gsize;
typedef gsize gssize;

#endif /* not GTYPES_H */
