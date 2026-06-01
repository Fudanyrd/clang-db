"""
Representations of C/C++ types.
"""

class TypeBase():
    """
    Base class for all C++ types. 
    """
    __slots__ = ['is_constant', 'is_volatile']
    def __init__(self, is_constant: bool = False, is_volatile: bool = False):
        self.is_constant = is_constant
        self.is_volatile = is_volatile

    def _fmt_lh(self, has_name: bool) -> str:
        """
        Format the left-hand (relative to the name) side of a type representation.
        For instance, type `int *[32]`'s left-hand side is `int (*`.
        """
        raise NotImplementedError("fmt() is not implemented for base class TypeBase")
        
    def _fmt_rh(self, has_name: bool) -> str:
        """
        Format the right-hand (relative to the name) side of a type representation.
        For instance, type `int *[32]`'s right-hand side is `) [32]`.
        """
        raise NotImplementedError("fmt() is not implemented for base class TypeBase")

    def fmt(self, name: str | None) -> str:
        """
        :param name: if it is set to None, then the type representation
        will be unnamed, e.g. int (*) (int). If it is set to a string, then the
        type representation will be named, e.g. int (*fn) (int).
        """
        if name is not None:
            return f'{self._fmt_lh(True)}{name}{self._fmt_rh(True)}'
        return f'{self._fmt_lh(False)}{self._fmt_rh(False)}'

    def __int__(self) -> int:
        """ It is needed for cache-ing built-in type objects. """
        return int(self.is_volatile) * 2 + int(self.is_constant)


class BuiltinType(TypeBase):
    __slots__ = ['idx'] + TypeBase.__slots__
    __kinds__ = [
        'signed char',
        'bool',
        'char',
        'double',
        'long double',
        'float',
        'float128',
        'unsigned char',
        'int',
        'unsigned int',
        'long',
        'unsigned long int',
        '__int128',
        'unsigned __int128',
        'short',
        'unsigned short',
        'void',    
        'wchar_t',
        'long long int',
        'unsigned long long int',
        '...',
        'decltype(nullptr)',
    ]
    def __init__(self, name: str, is_constant: bool = False, is_volatile: bool = False):
        super().__init__(is_constant, is_volatile)
        self.idx = BuiltinType.__kinds__.index(name)

    def _fmt_rh(self, has_name: bool) -> str:
        return ''

    def _fmt_lh(self, has_name: bool) -> str:
        if self.is_constant:
            suffix = " const"
        else:
            suffix = ""
        if self.is_volatile:
            suffix += " volatile"
        return self.__kinds__[self.idx] + suffix + ' '


# Layout of this cache:
# [ (no modification) ] + [ const ] + [ volatile ]  + [ const volatile ]
_builtin_type_cache: list[BuiltinType | None] = [None] * (len(BuiltinType.__kinds__) * 4)

def get_builtin_type(name: str, is_constant: bool = False, is_volatile: bool = False) -> BuiltinType:
    offset = BuiltinType.__kinds__.index(name)
    idx = (int(is_constant) + 2 * int(is_volatile)) # possible values: 0,1,2,3
    idx = idx * len(BuiltinType.__kinds__)  + offset
    if _builtin_type_cache[idx] is None:
        _builtin_type_cache[idx] = BuiltinType(name, is_constant, is_volatile)
    return _builtin_type_cache[idx]


def builtin_type_set_constant(orig_obj: BuiltinType) -> BuiltinType:
    if orig_obj.is_constant:
        return orig_obj
    return get_builtin_type(BuiltinType.__kinds__[orig_obj.idx], True, orig_obj.is_volatile)


def builtin_type_set_volatile(orig_obj: BuiltinType) -> BuiltinType:
    if orig_obj.is_volatile:
        return orig_obj
    return get_builtin_type(BuiltinType.__kinds__[orig_obj.idx], orig_obj.is_constant, True)


class RecordType(TypeBase):
    __slots__ = ['decl'] + TypeBase.__slots__

    def __init__(self, decl, is_constant: bool = False, is_volatile: bool = False):
        from .decl import RecordDecl
        super().__init__(is_constant, is_volatile)
        self.decl : RecordDecl = decl


class UnresolvedType(TypeBase):
    """
    Represents a RecordType whose declaration cannot be found.
    It is used in the first scans of declaration tree building.
    """
    __slots__ = ['mangled_name'] + TypeBase.__slots__
    def __init__(self, mangled_name: str, is_constant: bool = False, is_volatile: bool = False):
        super().__init__(is_constant, is_volatile)
        self.mangled_name = mangled_name
        print(mangled_name)

    def _fmt_lh(self, has_name: bool) -> str:
        return self.mangled_name + ' '
    
    def _fmt_rh(self, has_name: bool) -> str:
        return ''


class FunctionType(TypeBase):
    """
    Represent a function type, e.g. int (int, float).
    """
    __slots__ = ['return_type', 'param_types', 'template_parms'] + TypeBase.__slots__
    def __init__(self, return_type: TypeBase, param_types: list[TypeBase]):
        super().__init__(False, False) # const/volatile qualifier is not applicable to function type.
        self.return_type = return_type
        self.param_types = param_types

    def _fmt_lh(self, has_name: bool) -> str:
        ret = self.return_type._fmt_lh(False)
        ret += ' (' if has_name else ''
        return ret

    def _fmt_rh(self, has_name: bool) -> str:
        ret = ''
        if has_name:
            ret += ')'
        ret += ' ('
        for i, param in enumerate(self.param_types):
            if i > 0:
                ret += ', '
            ret += param.fmt(None)
        ret += ')'
        return ret


class PointerType(TypeBase):
    __slots__ = ["pointee_type"] + TypeBase.__slots__
    def __init__(self, pointee_type: TypeBase, is_constant: bool = False, is_volatile: bool = False):
        super().__init__(is_constant, is_volatile)
        self.pointee_type = pointee_type

    def _fmt_lh(self, has_name: bool) -> str:
        if self.is_constant:
            suffix = "const "
        else:
            suffix = ""
        if self.is_volatile:
            suffix += "volatile "
        return self.pointee_type._fmt_lh(True) + ' * ' + suffix

    def _fmt_rh(self, has_name: bool) -> str:
        return self.pointee_type._fmt_rh(True)



class ArrayType(TypeBase):
    __slots__ = ["element_type", "length"] + TypeBase.__slots__
    def __init__(self, element_type: TypeBase, length: int | None = None):
        super().__init__(False, False) # const/volatile qualifier is not applicable to array type.
        self.element_type = element_type
        self.length = length

    def _fmt_lh(self, has_name: bool) -> str:
        ret = self.element_type._fmt_lh(False)
        ret += ' (' if has_name else ''
        return ret

    def _fmt_rh(self, has_name: bool) -> str:
        ret = ''
        if has_name:
            ret += ')'
        if self.length is not None:
            ret += f' [{self.length}]'
        else:
            ret += ' []'
        ret += self.element_type._fmt_rh(False)
        return ret


class ReferenceType(TypeBase):
    __slots__ = ["refed_type"] + TypeBase.__slots__
    def __init__(self, refed_type: TypeBase):
        super().__init__(False, False) # const/volatile qualifier is not applicable to reference type.
        self.refed_type = refed_type

    def _fmt_lh(self, has_name: bool) -> str:
        # should treat referenced type as named,
        # because function type and array type will put parentheses around the name.
        return self.refed_type._fmt_lh(True) + ' &'
    
    def _fmt_rh(self, has_name: bool) -> str:
        return self.refed_type._fmt_rh(True)


class RvalueReferenceType(TypeBase):
    __slots__ = ["refed_type"] + TypeBase.__slots__
    def __init__(self, refed_type: TypeBase):
        super().__init__(False, False) # const/volatile qualifier is not applicable to reference type.
        self.refed_type = refed_type

    def _fmt_lh(self, has_name: bool) -> str:
        # should treat referenced type as named,
        # because function type and array type will put parentheses around the name.
        return self.refed_type._fmt_lh(True) + ' &&'

    def _fmt_rh(self, has_name: bool) -> str:
        return self.refed_type._fmt_rh(True)

