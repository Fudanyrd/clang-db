#!/usr/bin/sh
set -ex

test -d include || {
    echo "Error: include/ directory not found."
    exit 1
}

if test -z "$1"; then
    echo "Usage: $0 <project-name (identifier)>"
    exit 1
fi

cd include

sys_header="$1-sys.h"
input_header="$1.h"

toplevel_define=$(
python3 -c '
import sys, re
project_name = sys.argv[1]
if re.fullmatch("[_A-Za-z0-9]+", project_name) is None:
    print(f"Error: Invalid project name: {project_name}", file=sys.stderr)
    sys.exit(1)
print("__" + project_name.upper() + "_H__")
'  "$1"
)

# detect system headers.
echo '' > "$sys_header"
find . -name \*.h -print0 | xargs -0 cat | grep -E '#include <(.+)>' | sort | uniq > ./sys.inc

# Perform copy-paste preprocessing to resolve all #include "..." 
cat << EOF | python3 - "$input_header" > "$1.inc"
import re

processed = set()
include_pattern = re.compile(r'#include\s+"(.+)"')
sys_include_pattern = re.compile(r'#include\s+<(.+)>')

def preprocess(ifile: str):
    if ifile in processed:
        return
    processed.add(ifile)
    header_def = '__' + ifile.replace('/', '_').replace('.', '_').upper() + '__'
    exclude_stmts = [f'#ifndef {header_def}',
                     f'#define {header_def} (1)',
                     f'#endif /* {header_def} */']
    last_exclude_stmt = 0
    with open(ifile, 'r', encoding='utf-8') as f:
        lines = f.readlines()
        for line in lines:
            stripped = line.rstrip()
            if exclude_stmts[last_exclude_stmt] == stripped:
                last_exclude_stmt += 1
                if last_exclude_stmt == len(exclude_stmts):
                    print('\n')
                    break
                continue
            if re.match(sys_include_pattern, line):
                pass
            elif m := re.match(include_pattern, line):
                preprocess(m.group(1))
            else:
                print(line, end='')

if __name__ == '__main__':
    import sys
    input_header = sys.argv[1]
    preprocess(input_header)
EOF

# Generate the final header.
{
    echo '#ifndef' "$toplevel_define"
    echo '#define' "$toplevel_define" '(1)'
    echo '' # insert an empty line
} > "$sys_header"

{
  grep -v 'gtest' < sys.inc 
  cat < "$1.inc"
  echo '#endif' '/*' "$toplevel_define" '*/'
} >> "$sys_header"

rm -f sys.inc "$1.inc" || true

# finally, format the generated header.
cd ..
clang-format -i "include/$sys_header"

exit 0
