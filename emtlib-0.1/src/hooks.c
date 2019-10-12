/* Basic C hook implementation.  This is only example code!  Actually,
   this code could work pretty well on its own.  */

#include "hooks.h"

void init_hooks(hook_array hooks)
{
	EA_INIT(hook, hooks, 16);
}

void add_hook(hook_array hooks, genhook hook, context ctx_ptr)
{
	hooks.d[hooks.len].hook = hook;
	hooks.d[hooks.len].ctx = ctx_ptr;
}

void hook_anchor(hook_array hooks)
{
	unsigned i;
	for (i = 0; i < hooks.len; i++)
		hooks.d[i].hook(hooks.d[i].ctx);
}
