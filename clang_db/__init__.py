"""
### clang_db

A package for explaining the contents in the
SQLite database created by our project.

### A Quick Demo

>>> import sqlite
>>> # recommende to open the db read-only.
>>> db = sqlite.open('llvm.db', True)
>>> from clang_db.builder import Builder
>>> from clang_db.decl import *
>>> builder = Builder(db)
>>> tud: TranslationUnitDecl = builder.build()
"""

