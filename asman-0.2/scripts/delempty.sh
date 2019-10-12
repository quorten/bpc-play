#! /bin/sh
# Use GNU find to perform the equivalent of `delempty'.

if [ -n "$1" ]; then
  cd "$1"
  if [ $? -ne 0 ]; then
    exit 1
  fi
fi

find . -depth -type d -empty -delete
