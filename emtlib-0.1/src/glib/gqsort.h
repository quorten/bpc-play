#ifndef GQSORT_H
#define GQSORT_H

#include <glib/gtypes.h>

void g_qsort_with_data (guint8 *data, gint len, gsize size,
			GCompareDataFunc compare_func, gpointer user_data);

#endif
