/* Define a wrapper type for a GLib GArray.  */
#ifndef GA_WTYPE
#define GA_WTYPE(typename)						\
	struct typename##_array_tag					\
	{											\
		typename *d;							\
		unsigned len;							\
	};											\
	typedef struct typename##_array_tag typename##_array;
#endif
