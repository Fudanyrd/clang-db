import sys
from enum import Enum
from typing import Tuple

from .cctype import *
from .generic import *

def _safe_getitem(lst: list[str], idx: int) -> str:
    return lst[idx] if idx < len(lst) else ''


def strip_leading_digits(word: str) -> str:
    i = 0
    while i < len(word) and word[i].isdigit():
        i += 1
    return word[i:]

class DiagnosticSeverity(Enum):
    INFO = 0
    WARNING = 1
    ERROR = 2


DEMANGLE_OPERATOR: dict[str, str] = {
  "aN": "&=",
  "aS": "=",
  "dV": "/=",
  "eO": "^=",
  "lS": "<<=",
  "mI": "-=",
  "mL": "*=",
  "oR": "|=",
  "pL": "+=",
  "rM": "%=",
  "rS": ">>=",
  "an": "&",
  "co": "~",
  "dv": "/",
  "eo": "^",
  "eq": "==",
  "ge": ">=",
  "gt": ">",
  "le": "<=",
  "ls": "<<",
  "ls": "<",
  "mi": "-",
  "ml": "*",
  "mm": "--",
  "ne": "!=",
  "ng": "!",
  "or": "|",
  "pl": "+",
  "pp": "++",
  "ps": "%",
  "cl": "()",
  "ix": "[]",
  "nw": "new",
  "dl": "delete",
  "na": "new[]",
  "da": "delete[]",
  "pt": "->",
  "aa": "&&",
  "oo": "||",
  "rs": ">>",
  "cv": "/* convert */",
}

# 'Dn': decltype(nullptr) is not in this table
# because its unique length.
DEMANGLE_BUILTIN_TYPE: dict[str, str] = {
    'a': 'signed char',
    'b': 'bool',
    'c': 'char',
    'd': 'double',
    'e': 'long double',
    'f': 'float',
    'g': 'float128',
    'h': 'unsigned char',
    'i': 'int',
    'j': 'unsigned int',
    'l': 'long',
    'm': 'unsigned long int',
    'n': '__int128',
    'o': 'unsigned __int128',
    's': 'short',
    't': 'unsigned short',
    'v': 'void',    
    'w': 'wchar_t',
    'x': 'long long int',
    'y': 'unsigned long long int',
    'z': '...',
}

class Tokenizer():
    """
    The tokenizer recognizes elements in a mangled name and splits them.
    It shall only try to split the given input string without demangling it.

    Example:
    >>> t = Tokenizer('6public6static1i')
    >>> # tokenizer will split by length indicator
    >>> t.get()
    ['public', 'static', 'i']
    >>> Tokenizer('CDnz').get() # constructor (decltype(nullptr), ...)
    ['C', 'Dn', 'z']
    """

    __slots__ = ['source', '_pos', '_stk', '_result']
    __nestings__ = ['I', # start of template
                    'F', # start of function type
                    'N', # nested name
                    'L', # non-type template argument
                    'E']  # termniated by 'E'
    
    __modifiers__ = ['P', # pointer, e.g. int *
                     'R', # reference, e.g. int &
                     'O', # rvalue reference, e.g. int &&
                     'K', # const qualifier, e.g. const int
                     'V'] # volatile qualifier, e.g. volatile int
    def _pretty_diagnostics(self, what: str):
        print(f"In self.source:1:{self._pos}", file=sys.stderr)
        print("note:", self.source, file=sys.stderr)
        print(' ' * (5 + self._pos), '^', file=sys.stderr)
        raise ValueError(what)

    def _step(self) -> bool:
        """ Retrieve next token. """
        if self._stk and self._stk[-1] == 'L':
            # continuous digits are a value of non-type template argument.
            if self.source[self._pos].isdigit():
                _start = self._pos
                while self.source[self._pos].isdigit():
                    self._pos += 1
                self._result.append(self.source[_start:self._pos])
                return True

        if self._pos >= len(self.source):
            return False
        ch = self.source[self._pos]
        ch2 = self.source[self._pos:self._pos+2]
        if ch.isdigit():
            # length indicator, read the length and then read the token.
            length = 0
            while self._pos < len(self.source) and self.source[self._pos].isdigit():
                length = length * 10 + int(self.source[self._pos])
                self._pos += 1
            if self._pos + length > len(self.source):
                self._pretty_diagnostics("Invalid length indicator that leads to overflow")
            token = self.source[self._pos:self._pos+length]
            self._result.append(str(length) + token)
            self._pos += length
        elif ch in self.__nestings__:
            if ch == 'E':
                if not self._stk:
                    self._pretty_diagnostics("Unmatched 'E' found")
                self._stk.pop()
            else:
                self._stk.append(ch)
            self._result.append(ch)
            self._pos += 1
        # match overloaded operator
        elif ch == 'C':
            self._result.append('C') # constructor:
            self._pos += 1
        elif ch == 'D'  and len(self._result) == 0:
            self._result.append('D') # destructor
            self._pos += 1
        elif ch2 in DEMANGLE_OPERATOR and len(self._result) == 0:
            self._result.append(ch2)
            self._pos += 2
        # match builtin types
        elif ch2 == 'Dn':
            self._result.append('Dn')
            self._pos += 2
        elif ch in DEMANGLE_BUILTIN_TYPE:
            self._result.append(ch)
            self._pos += 1
        # match type modifier/quanlifiers
        elif ch in self.__modifiers__:
            self._result.append(ch)
            self._pos += 1
        # match array type, and do not sotre trailing '_'.
        # e.g. A42_ => tokens "A", "42"; "A_" => token "A".
        elif ch == 'A':
            self._result.append('A')
            self._pos += 1
            length = 0
            while self._pos < len(self.source) and self.source[self._pos].isdigit():
                length = length * 10 + int(self.source[self._pos])
                self._pos += 1
            if length > 0:
                self._result.append(str(length))
            if self._pos < len(self.source) and self.source[self._pos] == '_':
                self._pos += 1
        else:
            self._pretty_diagnostics(f"Unrecognized token starting with '{ch}'")

        return True

    def __init__(self, src: str):
        self. source = src
        self._pos = 0
        self._stk: list[str] = [] # a stack to memorize x('N', 'I', 'F') and 'E' pairs.
        self._result: list[str] = []
        while self._step(): pass
        if self._stk:
            self._pretty_diagnostics("Unmatched {I,F,N} to E found")

    def get(self) -> list[str]:
        return self._result

def _parser_pretty_diagnostics(tokens: list[str], idx: int, what: str, 
                               severity: DiagnosticSeverity = DiagnosticSeverity.ERROR):
    source = ''.join(tokens[:idx + 1])
    print(f"Near self.source:1:{len(source)}", file=sys.stderr)
    print("note:", source, file=sys.stderr)
    print(' ' * (5 + len(source) - len(tokens[idx])), '^', file=sys.stderr)
    if severity == DiagnosticSeverity.ERROR:
        raise ValueError(what)
    else:
        print("warning:", what, file=sys.stderr)


class TypeParser():
    """
    Given a list of tokens and a start position, this parser tries
    to create a TypeBase object that represents the type described by the tokens.

    After the first type object is yield, it will set pos to the position of the next token,
    so that the caller can continue parsing the next type if needed.
    """
    __slots__ = ["tokens", "_result", "pos"]

    def _pretty_diagnostics(self, idx: int, what: str, severity: DiagnosticSeverity = DiagnosticSeverity.ERROR):
        _parser_pretty_diagnostics(self.tokens, idx, what, severity)

    def _handle_function(self):
        tokens = self.tokens # alias self.tokens
        nxt = self.pos
        assert tokens[nxt] == 'F'

        ret_type = None
        param_types = []
        nxt += 1
        while tokens[nxt] != 'E':
            type_parser = TypeParser(tokens, nxt)
            if ret_type is None:
                ret_type = type_parser.get()
            else:
                param_types.append(type_parser.get())
            nxt = type_parser.pos

        self.pos = nxt + 1 # skip the 'E' that ends nested name.
        if ret_type is None:
            self._pretty_diagnostics(nxt, "Function type must have a return type")
        else:
            self._result = FunctionType(ret_type, param_types)

    def _handle_cxx_record(self):
        tokens = self.tokens
        assert tokens[self.pos] == 'N'
        start = self.pos
        nests = ['N']
        self.pos += 1
        while nests:
            if tokens[self.pos] in Tokenizer.__nestings__:
                if tokens[self.pos] == 'E':
                    # tokenizer has checked this for us,
                    # so we do not check again.
                    nests.pop()
                else:
                    nests.append(tokens[self.pos])

            self.pos += 1
        # cannot create RecordType because the decl tree
        # is not built.
        self._result = UnresolvedType(''.join(tokens[start : self.pos]))

    def __init__(self, tokens: list[str], pos: int = 0):
        self.tokens = tokens
        self._result = None
        self.pos = pos

        if pos >= len(tokens):
            self._pretty_diagnostics(pos, "Unexpected end of tokens found")

        if tokens[pos] == 'I':
            _parser_pretty_diagnostics(tokens, pos, "Unexpected template argument list found")
        # handle builtin type
        elif tokens[pos] in DEMANGLE_BUILTIN_TYPE:
            self._result = get_builtin_type(DEMANGLE_BUILTIN_TYPE[tokens[pos]])
            self.pos += 1
        elif tokens[pos] == 'Dn':
            self._result = get_builtin_type('decltype(nullptr)')
            self.pos += 1
        # handle pointer and reference.
        elif tokens[pos] == 'P':
            pointee_parser = TypeParser(tokens, pos + 1)
            self._result = PointerType(pointee_parser.get())
            self.pos = pointee_parser.pos
        elif tokens[pos] == 'R':
            refed_parser = TypeParser(tokens, pos + 1)
            self._result = ReferenceType(refed_parser.get())
            self.pos = refed_parser.pos
        elif tokens[pos] == 'O':
            refed_parser = TypeParser(tokens, pos + 1)
            self._result = RvalueReferenceType(refed_parser.get())
            self.pos = refed_parser.pos
        elif tokens[pos] == 'K':
            modified_type = TypeParser(tokens, pos + 1)
            orig_type = modified_type.get()
            if isinstance(orig_type, BuiltinType):
                self._result = builtin_type_set_constant(orig_type)
            else:
                self._result = orig_type
                self._result.is_constant = True
            self.pos = modified_type.pos
        elif tokens[pos] == 'V':
            modified_type = TypeParser(tokens, pos + 1)
            orig_type = modified_type.get()
            if isinstance(orig_type, BuiltinType):
                self._result = builtin_type_set_volatile(orig_type)
            else:
                self._result = orig_type
                self._result.is_volatile = True
            self.pos = modified_type.pos
        # handle function.
        elif tokens[pos] == 'F':
            self._handle_function()
        # handle cxx record.
        elif tokens[pos] == 'N':
            self._handle_cxx_record()
        # handle array.
        elif tokens[pos] == 'A':
            pos += 1
            if tokens[pos].isdigit():
                length = int(tokens[pos])
                pos += 1
            else:
                length = None
            element_type = TypeParser(tokens, pos)
            self._result = ArrayType(element_type.get(), length)
            self.pos = element_type.pos
        else:
            self._pretty_diagnostics(pos, f"Unrecognized token '{tokens[pos]}' found")

    def get(self) -> TypeBase:
        """
        Parse the tokens and return a TypeBase object that represents the type described by the tokens.
        """
        assert self._result, f'{self.tokens}'
        return self._result


def parse_template(tokens: list[str], pos: int) -> tuple[TemplateArgList, int]:
    from .decl import UNKNOWN_DECL
    assert tokens[pos] == 'I'
    pos += 1
    elements = []

    while tokens[pos] != 'E':
        if tokens[pos] == 'L':
            # non type argument/parameter
            pos += 1
            parser = TypeParser(tokens, pos)
            pos = parser.pos
            value = None
            if tokens[pos].isdigit():
                value = tokens[pos]; pos += 1
            else:
                pass # no value for the arg.
            elements.append(NonTypeTemplateParameter(value, False, UNKNOWN_DECL))
            pos += 1 # go past the 'E'.
        else:
            parser = TypeParser(tokens, pos)
            elements.append(parser.get())
            pos = parser.pos
        del parser

    pos += 1 # go past the 'E', which ends template arg.

    return TemplateArgList(elements), pos


def parse_baseclass_list(tokens: list[str], pos: int) -> tuple[list, int]:
    from .decl import Inheritance, AccessSpecifier, ACCESS_KEYWORDS
    # in clangdb C++: see TypeofCXXRecordDecl for base class encoding.
    assert tokens[pos] == '7virtual' # indicator of start of base class list.
    pos += 1

    access = AccessSpecifier.Public
    ret: list[Inheritance] = []
    while pos < len(tokens):
        token = strip_leading_digits(tokens[pos])
        if token in ACCESS_KEYWORDS:
            access = ACCESS_KEYWORDS[token]
        elif token == 'virtual':
            # (virtual inheritance) ignored.
            pass
        else:
            ret.append(Inheritance(UnresolvedType(token), access))
        pos += 1
    return ret, pos


def parse_function_parameters(tokens: list[str], pos: int) -> list[TypeBase]:
    ret = []
    while pos < len(tokens):
        parser = TypeParser(tokens, pos)
        ret.append(parser.get())
        pos = parser.pos
        del parser
    return ret
