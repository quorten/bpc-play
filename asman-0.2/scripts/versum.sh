#! /bin/sh
# Recursively verify md5sums in a directory.
# We use GNU find to avoid huge path names inside a single
# `md5sums.txt' file.

if [ -n "$1" ]; then
  cd "$1"
  if [ $? -ne 0 ]; then
    exit 1
  fi
fi

find . -name md5sums.txt -execdir \
  sh -c 'echo pwd: "$PWD"; md5sum -c -- "$@"' sh '{}' +
