# This GNU sed script can help you translate a GLib GArray to an
# exparray.

s/GArray *\*/char_array/g
s/\([^ ]*\) *= *g_array_new *(.*, *.*, *\(.*\))/\1 = malloc (sizeof(FIXME_array)); EA_INIT_FIXME (\2, 16, *\1)/g
s/\([^ ]*\) *= *g_array_sized_new *(.*, *.*, *\(.*\), *\(.*\))/\1 = malloc (sizeof(FIXME_array)); EA_INIT_FIXME (\2, 16, *\1); (*\1).num = \3; EA_NORMALIZE (*\1)/g
s/g_array_append_val *(\(.*\),/EA_APPEND (*\1,/g
s/g_array_append_vals *(\(.*\),/EA_APPEND_MULT (*\1,/g
s/g_array_prepend_val *(\(.*\),/EA_PREPEND (*\1,/g
s/g_array_prepend_vals *(\(.*\),/EA_PREPEND_MULT (*\1,/g
s/g_array_insert_val *(\(.*\),/EA_INSERT (*\1,/g
s/g_array_insert_vals *(\(.*\),/EA_INSERT_MULT (*\1,/g
s/g_array_remove_index *(\(.*\),/EA_REMOVE (*\1,/g
s/g_array_remove_index_fast *(\(.*\),/EA_REMOVE_FAST (*\1,/g
s/g_array_remove_range *(\(.*\),/EA_REMOVE_RANGE (*\1,/g
s/g_array_sort/sort_FIXME/g
s/g_array_sort_with_data/sort_with_data_FIXME/g
s/g_array_index *(\(.*\), *\(.*\), *\(.*\))/\&\1.d[\3]/g
s/g_array_set_size *(\(.*\),/EA_SET_SIZE (*\1,/g
s/g_array_free *(\(.*\))/EA_DESTROY (*\1); free(\1)/g
