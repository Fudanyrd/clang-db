#!/usr/bin/env sh

set -e

apt-get -y update
apt-get -y install \
  build-essential \
  cmake \
  python3-dev \
  python3-pip \
  clang-format-15 \
  clang-tidy-15 \
  doxygen \
  clang-15 \
  gcc \
  g++ \
  libclang-15-dev \
  gawk \
  unzip \
  wget \
  shellcheck

test -d /usr/bin || {
  mkdir -p /usr/bin
  export PATH="/usr/bin:$PATH"
}
which clang-format || {
  ln -sf "$( which clang-format-15 )" /usr/bin/clang-format
}
which clang-tidy || {
  ln -sf "$( which clang-tidy-15 )" /usr/bin/clang-tidy
}

# install our python extensions.
python3 -m pip install 3rdparty/pyptr
python3 -m pip install 3rdparty/sqlite

exit 0
