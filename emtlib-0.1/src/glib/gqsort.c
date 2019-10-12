/* An implementation for `g_qsort_with_data'.  */

#include <glib/gqsort.h>

static void ga_swap(guint8 *data, gsize size, guint first, guint second)
{
  /* Note that we don't have to swap the elements this way.
     We could do a for loop that does the magic XORs, one word
     at a time, unless the target compiler has a bug that
     would cause such code not to work.  */
  guint i;
  for (i = 0; i < size; i += sizeof(guint))
    {
      guint temp, *word1, *word2;
      word1 = (guint *) (&data[size*first+i]);
      word2 = (guint *) (&data[size*second+i]);
      temp = *word1;
      (*word1) = *word2;
      (*word2) = temp;
    }
  if (i > size)
    {
      /* Copy the last few bytes one at a time.  */
      for (i -= sizeof(guint); i < size; i++)
	{
	  guint8 temp, *byte1, *byte2;
	  byte1 = &data[size*first+i];
	  byte2 = &data[size*second+i];
	  temp = *byte1;
	  (*byte1) = *byte2;
	  (*byte2) = temp;
	}
    }
}

void g_qsort_with_data (guint8 *data, gint len, gsize size,
			GCompareDataFunc compare_func, gpointer user_data)
{
  guint pivot, i, j;
  if (len <= 0)
    return;

  /* Choose the pivot.  */
  pivot = len / 2;

  /* Partition the array.  */
  i = 0;
  j = len - 1;
  while (i <= j)
    {
      if (compare_func(&data[size*i], &data[size*pivot], user_data) < 0)
	i++;
      else if (compare_func(&data[size*pivot], &data[size*j], user_data) < 0)
	j--;
      else
	{
	  ga_swap(data, size, i, j);
	  i++;
	  j--;
	}
    }

  /* `i' and `j' have just crossed each other.  `i' is equal to the
     value larger than the pivot and `j' is equal to the value smaller
     than the pivot.  */

  /* Make sure the pivot is in the correct position.  */
  if (j > pivot)
    {
      ga_swap(data, size, j, pivot);
      pivot = j;
    }
  else if (pivot > i)
    {
      ga_swap(data, size, pivot, i);
      pivot = i;
    }

  /* Sort recursively.  */
  g_qsort_with_data(data, pivot, size, compare_func, user_data);
  g_qsort_with_data(&data[pivot+1], len - (pivot + 1), size, compare_func,
		    user_data);
}
