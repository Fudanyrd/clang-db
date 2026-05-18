#!/usr/bin/sh 

set -ex

config_file="$PWD/.clang-format"

do_fmt() {
  find . \( -name \*.cc -or -name \*.h \) -print0 | xargs -0 clang-format --style="file:$config_file" -i
}

# shellcheck disable=SC2043
for srcdir in include src test bin; do
  cd "$srcdir"
  do_fmt
  cd ..
done

# 3rdparty/sqlite: only format our code.
clang-format --style="file:$config_file" -i 3rdparty/sqlite/*.cc 3rdparty/sqlite/*.hh

exit 0

