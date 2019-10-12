#! /bin/sh
mkdir t1
touch --sum 0004 ahead
touch --sum 0001 t1/b
touch --sum 0002 t1/a
touch --sum 0003 t1/c
mv t1/ tbasi/
