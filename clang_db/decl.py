import pyptr
from enum import Enum

from .cctype import TypeBase, FunctionType

class DeclContextKind(Enum):
    TranslationUnit = 0
    Namespace = 1
    Record = 2
    Field = 3
    Function = 4

class RecordKind(Enum):
    Struct = 1
    Class = 2
    Union = 3
    Enum = 4

class AccessSpecifier(Enum):
    Unknown = 0
    Private = 1
    Protected = 2
    Public = 3

ACCESS_KEYWORDS = {
    'public': AccessSpecifier.Public,
    'private': AccessSpecifier.Private,
    'protected': AccessSpecifier.Protected,
}

class DeclContext():
    __slots__ = ['parent', 'kind', 'children']

    def __init__(self, kind: DeclContextKind, parent):
        self.parent: pyptr.PyPointer = pyptr.PyPointer(parent)
        self.kind = kind
        self.children: list[DeclContext] = []

    def fix_unresolved(self, impl, tud) -> None:
        """
        Helper method used only in UnresolvedType resolution.
        Defaults to doing nothing.

        :param impl: resolution implementation, e.g. builder.resolve_record_type
        :param tud: the translation unit declaration.
        """
        pass

class TranslationUnitDecl(DeclContext):
    __slots__ = DeclContext.__slots__

    def __init__(self):
        super().__init__(DeclContextKind.TranslationUnit, None)


UNKNOWN_DECL: DeclContext =  TranslationUnitDecl()


class NamespaceDecl(DeclContext):
    __slots__ = ['name'] + DeclContext.__slots__

    def __init__(self, name: str, decl_context: DeclContext):
        super().__init__(DeclContextKind.Namespace, decl_context)
        self.name = name


class Inheritance():
    __slots__ = ['base', 'access']
    from .cctype import UnresolvedType, RecordType

    def __init__(self, base_type: UnresolvedType | RecordType, access: AccessSpecifier):
        self.base = base_type
        self.access = access


class RecordDecl(DeclContext):
    __slots__ = ['name', 'record_kind', 'template_args', 'access', 'base_classes'] + DeclContext.__slots__

    def __init__(self, name: str, 
                 record_kind: RecordKind, 
                 decl_context: DeclContext,
                 access: AccessSpecifier = AccessSpecifier.Unknown,
                 base_classes: list[Inheritance] = [],
                 template_args = None):
        from .generic import TemplateArgList
        super().__init__(DeclContextKind.Record, decl_context)
        self.name = name
        self.record_kind = record_kind
        self.template_args : TemplateArgList | None = template_args
        self.base_classes = base_classes
        self.access = access

    def fix_unresolved(self, impl, tud) -> None:
        for base in self.base_classes:
            base.base = base.base.fix_unresolved(impl, tud)


class FieldDecl(DeclContext):
    __slots__ = ['name', 'type', 'access'] + DeclContext.__slots__

    def __init__(self, name: str, type: TypeBase, 
                 decl_context: DeclContext, 
                 access: AccessSpecifier = AccessSpecifier.Unknown):
        super().__init__(DeclContextKind.Field, decl_context)
        self.name = name
        self.type = type
        self.access = access

    def fix_unresolved(self, impl, tud) -> None:
        self.type = self.type.fix_unresolved(impl, tud)

class FunctionDecl(DeclContext):
    __slots__ = ['name', 'ftype', 'access', 'template_args'] + DeclContext.__slots__

    def __init__(self, name: str, ftype: FunctionType, 
                 decl_context: DeclContext, 
                 access: AccessSpecifier = AccessSpecifier.Unknown,
                 template_args = None):
        from .generic import TemplateArgList
        super().__init__(DeclContextKind.Function, decl_context)
        self.name = name
        self.ftype = ftype
        self.access = access
        self.template_args: TemplateArgList | None = template_args

    def fix_unresolved(self, impl, tud) -> None:
        self.ftype = self.ftype.fix_unresolved(impl, tud)
