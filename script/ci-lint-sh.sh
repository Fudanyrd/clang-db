#!/usr/bin/env sh
set -e

for f in ./script/*.sh; do
  shellcheck "$f"
done

exit 0
