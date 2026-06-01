"""
Represents concepts of C++ template:
  - template parameter
  - template argument
  - non-type parameter (e.g. std::array<int, 4>)
"""

from .decl import DeclContext
from .cctype import TypeBase
import pyptr

class TemplateParameter():
    __slots__ = ['name', 'tyobj', 'is_variadic', 'decl']
    class TemplateParmType(TypeBase):
        __slots__ = ['template_parameter', 'is_pack'] + TypeBase.__slots__
        def __init__(self, template_parameter,
                     is_pack: bool = False,
                     is_const: bool = False, is_volatile: bool = False):
            super().__init__(is_const, is_volatile)
            self.is_pack = is_pack
            self.template_parameter = pyptr.PyPointer(template_parameter)

        def _fmt_lh(self, has_name: bool) -> str:
            suffix = ' ' if not self.is_constant else ' const '
            if self.is_volatile:
                suffix += ' volatile '
            parm: TemplateParameter = self.template_parameter()
            return parm.name + suffix

        def _fmt_rh(self, has_name: bool) -> str:
            return ''

    def __init__(self, name: str, is_variadic: bool, decl: DeclContext):
        self.name = name
        self.is_variadic = is_variadic
        self.decl = pyptr.PyPointer(decl)
        self.tyobj = TemplateParameter.TemplateParmType(self)

    def fmt(self):
        return self.name + ('...' if self.is_variadic else '')

TemplateParameterType = TemplateParameter.TemplateParmType

class NonTypeTemplateParameter():
    __slots__ = ['value', 'is_variadic', 'decl']
    def __init__(self, value: str | None, is_variadic: bool, decl: DeclContext):
        self.value = value
        self.is_variadic = is_variadic
        self.decl = pyptr.PyPointer(decl)

    def fmt(self):
        if self.value:
            return self.value
        else:
            return 'UDF'


class TemplateArgList():
    """
    Representing all kinds of template argument list/template parameter list.
    For instance:
    ```c++
    template <typename K, typename V> struct Map { }; // parameter list
    template <typename V> struct Map<int, V> { };  // partial specialization argument list
    ```
    """
    __slots__ = ['args']
    def __init__(self, args: list[TemplateParameter | NonTypeTemplateParameter | TypeBase]):
        self.args = args

    def fmt(self) -> str:
        return ', '.join((arg.fmt(None) if isinstance(arg, TypeBase) else arg.fmt()) for arg in self.args)

    def __repr__(self) -> str:
        return f'<{self.fmt()}>'

