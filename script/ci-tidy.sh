#!/usr/bin/sh
# run clang-tidy on the codebase.
set -e

test -f '.clang-tidy' || {
    echo "Error: .clang-tidy file not found in the current directory." >&2
    exit 1
}

workdir="$( pwd )"
compile_commands="$workdir/build/compile_commands.json"
test -f "$compile_commands" || {
    echo "Error: compile_commands.json not found in $workdir/build." >&2
    exit 1
}

for f in test/*.cc; do
    echo "$f" 1>&2
    clang-tidy -p "$compile_commands" "$f"
done

exit 0
