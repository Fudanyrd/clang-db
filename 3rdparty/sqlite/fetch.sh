#!/usr/bin/sh

set -ex

{
  test -f sqlite3.c && exit 0
} || true

url='https://sqlite.org/2026/sqlite-amalgamation-3530100.zip'
wget "$url" -O sqlite.zip
unzip -d . sqlite.zip 
mv -f sqlite-amalgamation-3530100/*.h .
mv -f sqlite-amalgamation-3530100/*.c .
rm -rf sqlite-amalgamation-3530100 sqlite.zip
rm -f shell.c

test -f .gitignore || {
  printf '%s\n%s\n%s\n' sqlite3.c sqlite3.h sqlite3ext.h > .gitignore
}

exit 0

