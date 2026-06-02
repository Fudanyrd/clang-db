#!/usr/bin/env sh
# Run regression testing.

set -e

build_dir="build"
if test -n "$1"; then
  build_dir="$1"
  echo "Using custom build directory: $build_dir" 1>&2
fi

test -d "$build_dir" || ( echo "Error: build directory $build_dir does not exist" 1>&2 && exit 1 )
clang_plugin="$( realpath "$build_dir"/bin/BuildDatabase.so )"

if test -z "$CC"; then
  CC=clang-15
fi

for testfile in test/resource/*.cc; do 
  # run plugin for the database file.
  "$CC" -cc1 -load "$clang_plugin" \
  -plugin build-db \
  -plugin-arg-build-db -o="$testfile.db" \
  -std=c++11 "$testfile";

  # use a script to verify the content.
  python3 - "$testfile.db" < script/check-db.py

  # clean up.
  rm -f "$testfile.db" || true
done

exit 0
