#!/usr/bin/sh

set -ex

config_file="$PWD/.clang-format"

do_check() {
  find . \( -name \*.cc -or -name \*.h \) -print0 |
    xargs -0 clang-format --style="file:$config_file" --dry-run --Werror
}

for srcdir in include src test; do
  cd "$srcdir"
  do_check
  cd ..
done

exit 0
