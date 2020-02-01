#! /bin/sh
# Take a tarball, directory, etc. and turn it into a Git repository,
# preserving file timestamps.

if [ $# -eq 0 ]; then
  echo git init .
  $0 first run | sort
  exit
fi

if [ $# -eq 2 ]; then
  find -name .git -prune -o -not -type d \
    -exec $0 {} \;
fi

if [ $# -eq 1 ]; then
  ls --time-style="+%F %T %z" -l "$1" | awk '{ print "GIT_AUTHOR_DATE=\""$6" "$7" "$8"\" GIT_COMMITTER_DATE=\""$6" "$7" "$8"\" git add \"'"$1"'\" && GIT_AUTHOR_DATE=\""$6" "$7" "$8"\" GIT_COMMITTER_DATE=\""$6" "$7" "$8"\" git commit -m \"Save file.\" \"'"$1"'\"" }'
  exit
fi

exit

# PLEASE NOTE: After clone, you must run the following script to
# restore timestamps:

IFS="
"
for FILE in $(git ls-files)
do
    TIME=$(git log --pretty=format:%cd -n 1 --date=iso -- "$FILE")
    touch -m --date "$TIME" "$FILE"
done

# 20200128/DuckDuckGo git checkout files with date  
# 20200128/DuckDuckGo git checkout preserve timestamps  
# 20200128/https://stackoverflow.com/questions/2179722/checking-out-old-file-with-original-create-modified-timestamps
