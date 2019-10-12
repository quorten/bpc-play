#! /bin/sh
# Recursively save md5sums in a directory.
# We use GNU find to avoid huge path names inside a single
# `md5sums.txt' file.

if [ -n "$1" ]; then
  cd "$1"
  if [ $? -ne 0 ]; then
    exit 1
  fi
fi

find . -name md5sums.txt -delete
find . -type f -execdir \
  sh -c 'md5sum -- "$@" >>md5sums.txt' sh '{}' +
