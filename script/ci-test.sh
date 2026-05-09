#!/usr/bin/env sh
# Usage:
# CC={C compiler} CXX={CXX Compiler} [CXXSTD={11,14,17,20}] ci-test.sh [build_dir]

set -ex
( tty && clear ) || true

# check that CC and CXX are set
test -n "$CC"
test -n "$CXX"

build_dir="build"
if test -n "$1"; then
  build_dir="$1"
  echo "Using custom build directory: $build_dir" 1>&2
fi

install_dir=".install"
if test "$install_dir" == "$build_dir"; then
  echo "Error: build directory cannot be the same as install directory" 1>&2
  exit 1
fi
mkdir -p "$install_dir"
install_dir="$( realpath $install_dir )"

if test -n "$CXXSTD"; then
  cxx_std_flag="-DCMAKE_CXX_STANDARD=$CXXSTD"
else
  cxx_std_flag=""
fi

mkdir -p "$build_dir" && cd "$build_dir"

# When CXXSTD is not set, use cmake default, and cxx_std_flag should
# not be double-quoted.
# shellcheck disable=SC2086
cmake .. -DCMAKE_BUILD_TYPE=Release \
  $cxx_std_flag \
  -DCLANGDB_BUILD_UNITTESTS=TRUE \
  -DCMAKE_INSTALL_PREFIX="$install_dir" \
  -DCMAKE_C_COMPILER="$CC" \
  -DCMAKE_CXX_COMPILER="$CXX"

make -j"$(nproc)"
make run-tests

# finally, test that the project can be installed.
# [disabled at project startup]
# make install && test -d "$install_dir/lib" && test -d "$install_dir/include"
exit 0
