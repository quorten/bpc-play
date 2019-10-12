incdir=$1
libdir=$2
linkarch=$3
cat <<EOF
%rename cpp_options old_cpp_options

*cpp_options:
-nostdinc -isystem $incdir -isystem include%s %(old_cpp_options)

*cc1:
%(cc1_cpu) -nostdinc -isystem $incdir -isystem include%s

*link_libgcc:
-L$libdir -L .%s

*libgcc:


*startfile:
$libdir/crt0.o

*endfile:


*link:
$linkarch -nostdlib %{shared:-shared} %{static:-static} %{rdynamic:-export-dynamic}

*esp_link:


*esp_options:


*esp_cpp_options:


EOF
