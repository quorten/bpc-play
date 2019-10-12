# Update script header
while [ -n "${1}" ]; do
  case "${1}" in
      -SRC) shift; SRC=${1};;
      -DEST) shift; DEST=${1};;
      -h | --help)
	  echo Usage: ${0} [-SRC SRC_ROOT] [-DEST DEST_ROOT]
	  cat <<EOF
SRC_ROOT and DEST_ROOT may be specified to override the defaults.
EOF
	  exit 0;;
      *) echo Invalid argument: ${1}
	 echo Type \'${0} --help\' for help.
	 exit 1;;
  esac
  shift
done
# End of header
