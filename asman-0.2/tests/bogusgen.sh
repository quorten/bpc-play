#! /bin/sh
mkdir bogus
cd bogus
mkdir -p dir3/dir6/dir8/dir10/dir11/dir12/dir13
mkdir -p dir1/dir2
mkdir -p dir3/dir7
echo "Don't delete me!  I'm not empty!" > dir3/dir7/info
mkdir -p dir3/dir6/dir9
mkdir -p dir3/dir5
mkdir -p dir3/dir4
mkdir -p "super
bad"
