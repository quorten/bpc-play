#! /bin/sh

mkdir battle
cd battle
cat <<EOF > badfile
This is a very bad file.
It should be deleted, shouldn't it?
EOF
cat <<EOF > goodfile
This is a good file.
It should dominate over the evil files.
EOF
cp badfile evilfile

mkdir subdir
cat <<EOF > subdir/other
This is a file for testing a hard link backup.
EOF
cat <<EOF > subdir/another
Wow!  Another file for testing hard link backups.
EOF
cp subdir/another more
cp subdir/another bundles
cp subdir/another oodles
cp subdir/another too-much
cp badfile subdir/legion

mkdir subdir/last
cat <<EOF > subdir/last/note
And here is one last subdirectory for testing.
EOF

mkdir restricted
cat <<EOF > restricted/prop1
Adobe Acrobat Installer
EOF
cat <<EOF > restricted/prop2
Intel(R) Architecture Software Developer's Manual
EOF

cat <<EOF > 'super crazy	ultra $`\"
bad'
This is a horrible file name.
EOF
