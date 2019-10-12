# -*- gdb-script -*- definitions for debugging expandable arrays.
# This file is in Public Domain.

define print_exparray
 set $i = 0
 while $i < $arg0->len
  print $arg0->d[$i++]
 end
end
document print_exparray
Print the contents of an expandable array.  Only one argument should
be given.
end

define p_ea
 print_exparray $arg0
end
document p_ea
Print the contents of an expandable array.  Only one argument should
be given.
end

define p_sa
 print *$arg0->d@$arg0->len
end
document p_sa
Print the contents of an expandable array by using GDB's artificial
array support.  You should probably only use this function for
debugging short and simple arrays, hence the name "p_sa".
end
