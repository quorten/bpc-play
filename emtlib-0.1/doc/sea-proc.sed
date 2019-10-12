# Process exparray.h.in to correspond to the implementation that is
# simple to the user, but possibly more complicated to the computer,
# depending on the compiler used.

2,4d
s/(typename, stride, /(/g
