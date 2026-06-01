"""
The declaration tree builder.
"""

from sqlite import (
    Database, Stmt, 
    SQLITE_OK, SQLITE_ROW, SQLITE_DONE, SQLITE_MISUSE
)

from .cctype import *
from .decl import *
from .parsing import (
  strip_leading_digits,  
  Tokenizer,
  TypeParser,
  DEMANGLE_OPERATOR,
  parse_template,
  parse_baseclass_list,
  parse_function_parameters,
)

from .common import safe_index

class Builder():
    __slots__ = ['db', 'str_stack', 'nested_decls']
    def __init__(self, db: Database):
        self.db = db
        self.str_stack: list[str] = []
        self.nested_decls: list[DeclContext] = []

    def _fetch_member_and_type(self) -> Stmt:
        table = 'class' if isinstance(self.nested_decls[-1], RecordDecl) else 'namespace'
        sql = f"SELECT distinct(member), type FROM {table} WHERE name = ?"
        stmt = self.db.prepare(sql)
        if not stmt:
            raise RuntimeError(f"Failed to prepare statement for retrieving members of {table} {self.str_stack[-1]}")
        if stmt.bind('s', (''.join(self.str_stack), )) != SQLITE_OK:
            raise RuntimeError(f"Failed to bind parameter for fetching members of {table} {self.str_stack[-1]}")
        return stmt

    def yield_decl(self, member: str, mtype: str) -> DeclContext:
        parent_decl  = self.nested_decls[-1]
        if mtype == '9namespace':
            return NamespaceDecl(strip_leading_digits(member), parent_decl)
        
        access = AccessSpecifier.Unknown
        kind: RecordKind | None = None
        mtype_tokens = Tokenizer(mtype).get()
        member_tokens = Tokenizer(member).get()

        is_static = False # 'static'
        is_mutable = False # 'mutable'

        for token in mtype_tokens:
            if token[1:] in ACCESS_KEYWORDS:
                access = ACCESS_KEYWORDS[token[1:]]
            elif token == '6struct':
                kind = RecordKind.Struct
            elif token == '5class':
                kind = RecordKind.Class
            elif token == '5union':
                kind = RecordKind.Union
            elif token == '4enum':
                kind = RecordKind.Enum
            elif token == '6static':
                is_static = True
            elif token == '7mutable':
                is_mutable = True
            elif token == '7typedef':
                return None # ignore typedefs for now

        name = member_tokens[0]
        next_pos = 1
        if member_tokens[0] == 'cv':
            parser = TypeParser(member_tokens, 1)
            name = 'operator' + DEMANGLE_OPERATOR['cv']
            # go past the converted to type, which can
            # be retrieved from the return value of the operator function.
            next_pos = parser.pos
        elif member_tokens[0] in DEMANGLE_OPERATOR:
            name = 'operator' + DEMANGLE_OPERATOR[member_tokens[0]]
        else:
            name = strip_leading_digits(name)

        if len(member_tokens) > next_pos and member_tokens[next_pos] == 'I':
            template_args, next_pos = parse_template(member_tokens, next_pos)
        else:
            template_args = None

        # test enum constant decl (TypeofEnumConstDecl)
        if mtype == '6public4enum1i':
            enum_decl = parent_decl
            assert isinstance(enum_decl, RecordDecl) and enum_decl.record_kind == RecordKind.Enum
            return FieldDecl(name, RecordType(enum_decl), enum_decl, access = AccessSpecifier.Public)

        # test cxx record declaration.
        if kind is not None:
            virt_idx = safe_index(mtype_tokens, '7virtual')
            if virt_idx is not None:
                base_classes, _ = parse_baseclass_list(mtype_tokens, virt_idx)
            else:
                base_classes = []

            return RecordDecl(name, kind, parent_decl, 
                              access = access,
                              base_classes = base_classes,
                              template_args = template_args)

        # test function: has at least one parameter.
        mtype_str = strip_leading_digits(mtype_tokens[-1])
        if next_pos < len(member_tokens):
            param_types = parse_function_parameters(member_tokens, next_pos)
            if parent_decl.kind.value >= DeclContextKind.Record.value:
                ret_type = TypeParser(Tokenizer(mtype_str).get(), 0).get()
            else:
                ret_type = TypeParser(mtype_tokens).get()

            ftype: FunctionType = FunctionType(ret_type, param_types)
            return FunctionDecl(name, ftype, parent_decl, access = access, template_args = template_args)

        field_type = TypeParser(Tokenizer(mtype_str).get(), 0).get()
        return FieldDecl(name, field_type, parent_decl, access = access)

    def _build_recurse(self, ctx: DeclContext, name: str):
        self.nested_decls.append(ctx) 
        self.str_stack.append(name) 

        stmt = self._fetch_member_and_type()
        while stmt.step() == SQLITE_ROW:
            member, mtype = stmt.get_columns('ss')
            next_decl = self.yield_decl(member, mtype)
            if next_decl is None:
                continue
            ctx.children.append(next_decl)
            if next_decl.kind.value <= DeclContextKind.Record.value:
                self._build_recurse(next_decl, member)
        self.nested_decls.pop()
        self.str_stack.pop()

    def build(self) -> TranslationUnitDecl:
        toplevel = TranslationUnitDecl()
        self._build_recurse(toplevel, '')
        return toplevel
