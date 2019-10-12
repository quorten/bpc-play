/* Basic C hook implementation.  This is only example code!  Actually,
   this code could work pretty well on its own.  */

#ifndef HOOKS_H
#define HOOKS_H

#include "exparray.h"

typedef void *context;
typedef void (*genhook)(context);

struct hook_tag
{
	genhook hook;
	context ctx;
};
typedef struct hook_tag hook;

EA_TYPE(hook);

void init_hooks(hook_array hooks);
void add_hook(hook_array hooks, genhook hook, context ctx_ptr);
void hook_anchor(hook_array hooks);

#endif /* not HOOKS_H */
