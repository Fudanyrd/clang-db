# Verify the integrity of generated database file,
# by trying to re-construct the declaration tree.
#
# Usage: check-db.py [clang database file]
import sqlite
import sys
from clang_db.builder import Builder, resolve_unresolved_types
from clang_db.decl import *

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <db_path>')
        sys.exit(1)
    db = sqlite.open(sys.argv[1], True)
    b = Builder(db)

    t: TranslationUnitDecl = b.build()
    db.close()
    resolve_unresolved_types(t)
