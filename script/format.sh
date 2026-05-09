#!/usr/bin/sh 

set -ex

config_file="$PWD/.clang-format"

do_fmt() {
  find . \( -name \*.cc -or -name \*.h \) -print0 | xargs -0 clang-format --style="file:$config_file" -i
}

# shellcheck disable=SC2043
for srcdir in test; do
  cd "$srcdir"
  do_fmt
  cd ..
done

exit 0

