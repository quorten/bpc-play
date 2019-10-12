# Process exparray.h.in to correspond to the implementation that is
# simple to dumb compilers, but more repetitive to program.

s/A simple expandable array/A fast expandable array/
2,4d
/#define TYPECAST void/d
/unsigned tysize/d
/unsigned stride/d
/(array).tysize = sizeof(typename)/d
/(array).stride = _stride/d
s/TYPECAST/typename/g
s/_stride/stride/g
s/(array).tysize/sizeof(typename)/g
s/(array).stride/stride/g
/#ifdef EA_FUNCTIONS/,/#endif \/\* EA_FUNCTIONS \*\//d
