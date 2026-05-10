# Clang Database Project

## What This Is

A light-weight C/C++ repository-level symbol indexing tool, based on
clang frontend.

## Testing

```sh
export CC=gcc # set C compiler
export CXX=g++ # set C++ compiler
./script/format.sh # format source code
./script/ci-test.sh # run tests
```

# High Level Ideas

## Dependencies

clang -- the frontend for AST
sqlite (TBD) -- manage database tables and indices.

## Database Schema

```sql
CREATE TABLE class(name VARCHAR,
                   member VARCHAR, -- data or method
                   type VARCHAR);

CREATE TABLE namespace(name VARCHAR,
                      member VARCHAR, -- data, other class or function
                      type VARCHAR);

-- map symbol (class, function, member) to its 
-- location of definition in source code.
--
-- namespaces are not included because they don't 
-- have a location in source code.
CREATE TABLE location(symbol VARCHAR,
                      file VARCHAR,
                      line INTEGER);
```

## Formalization

To avoid ambiguity in description, we use python
pseudo code to describe implementation details.

Some auxiliary functions or constants:

```py
# Map builtin type to its mangled name.
MANGLE_BUILTIN_TYPE = {'int': 'i', 'float': 'f', ...}

# Namespace encoding: 'foo::bar::baz' -> '3foo3bar3baz'
def encode_ns(*namespaces):
  return ''.join((len(ns) + ns) for ns in namespaces)
```


## Name and Member Column Spec

Normally, the `name` column stores the **mangled wholename**;
the `member` column stores its **mangled shortname**.

Their implementations in pseudo code:

```py
class Type(): 
  # returns partially mangled wholename
  def wholename(self) -> str: raise NotImplemented

  # returns short name (i.e. does not preserve the namespace info)
  def shortname(self) -> str: raise NotImplemented

# base class of template arguments.
class TemplateArg(): 
  def mangle(self) -> str:...
  def wholename(self) -> str: return self.mangle()


# a template type argument like `<size_t N>'
# Not implemented.
class TemplateNoneTypeArg(TemplateArg): ...

# a template type argument like `<typename T>'
class TemplateTypeArg(TemplateArg):
  __slots__ = (
    'name', # the token after `typename` or `class`.
    'default_value', # Type or None
  )
  
  # Objective: reserve the template info,
  # we convert the argument `<typename T>' to `<template::typename::T>'
  # and `<typename D = int>' to <template::typename::T::default::int>'
  # before mangling.
  def mangle(self) -> str:
    ret = 'N'+ encode_ns('template', 'typename') # template::typename::
    ret += encode_ns(self.name) # T::
    if self.default_value:
      ret += encode_ns('default')
      ret += encode_ns(self.default_value.wholename()) # default::SomeTy
    return ret + 'E' # close this

# builtin types. See .vscode/builtin.csv for full list.
class BuiltinType(Type):
  __slots__ = (
    'typename' # string, one of C/C++'s builtin types.
  )

  def wholename(self) -> str: 
    return MANGLE_BUILTIN_TYPE[self.typename]

  def shortname(self) -> str: return self.wholename()

# (C++) namespaces.
class Namespace(Type):
  __slots__ = (
    'level', # list[str], nested namespaces (including itself.)
  )

  def shortname(self):
    if not self.level:
      return '' # global namespace
    return encode_ns(self.level[-1])

  def wholename(self):
    if not self.level:
      return '' # global namespace
    return encode_ns(*self.level)

# class or struct.
class Class(Type):
  __slots__ = (
    'namespace', # Namespace, the mangled namespace or class it is in.
    'name', # name of the class.
    'template_args', # list[TemplateArg], template arguments if it's a template class.
  )

  def wholename(self) -> str:
    return self.namespace.wholename() + self.shortname()

  def shortname(self) -> str:
    if self.template_args:
      # e.g. tuple<int,int>
      return (encode_ns(self.name) +  # tuple
        'I' +  # <
        ''.join(arg.mangle() 
                for arg in self.template_args) # int,int
        + 'E') # >, close this template
    return encode_ns(self.name)


# function or method
class Function(Type):
  __slots__ = (
    'namespace', # Class | Namespace, where it came from
    'return_type', # Type
    'name', # str, name of the function.
    'paramaters', # list[Type | TemplateArg], parameter types.
    'template_args', # list[TemplateArg], template arguments if it's a template function.
  )

  # e.g. `N3foo3bar1fv' for `foo::bar::f(void)'
  def wholename(self) -> str:
    if self.template_args:
      # template function
      # e.g. `void foo::f<int>(int, int)' => `N3foo1fIiEEvii'
      return (
        'N' + self.namespace.wholename() +  # N3foo
        encode_ns(self.name) +  # 1f
        'I' +  # <
        ''.join(arg.mangle() 
                for arg in self.template_args) +  # i
        'EE' +  # >, close this template and this function
        self.return_type.wholename() +  # v
        ''.join(tp.wholename() for tp in self.parameters) # ii
      )
    return 'N' + self.namespace.wholename() + encode_ns(self.name) \
      + 'E' + ''.join(tp.wholename() for tp in self.parameters)

  # e.g. `1fv' for `foo::bar::f(void)'
  def shortname(self) -> str:
    if self.template_args:
      return (encode_ns(self.name) +  # 1f
        'I' +  # <
        ''.join(arg.mangle() 
                for arg in self.template_args) +  # i
        'E' +  # >, close this template
        self.return_type.wholename() +  # v
        ''.join(tp.wholename() for tp in self.parameters)) # ii
    return encode_ns(self.name) + \
        ''.join(tp.wholename() for tp in self.parameters)


class DataMember(Type):
  __slots__ = (
    'namespace', # Class | Namespace, where it came from
    'name', # str, name of the data member.
    'type_', # Type, type of this data member.
    'access', # Literal[None, 'public', 'protected', 'private']
  )
  # self.access is None if and only if it in a namespace, not class.

  def wholename(self) -> str:
    return self.namespace.wholename() + self.shortname()

  def shortname(self) -> str:
    return encode_ns(self.name)

# do not need union/enum, etc.
```


## Type Column Spec

These apply to all tables with the `type` column.

<table>
  <tr>
    <td>namespace</td>
    <td>its whole name</td>
  </tr>
  <tr>
    <td>class</td>
    <td>its whole name</td>
  </tr>
  <tr>
    <td>function</td>
    <td>its return type</td>
  </tr>
  <tr>
    <td>data member of class</td>
    <td>its decltype</td>
  </tr>
  <tr>
    <td>global variable</td>
    <td>its decltype</td>
  </tr>
  <tr>
    <td>member function (of class)</td>
    <td>See (1)</td>
  </tr>
  <tr>
    <td>C symbols</td>
    <td>See (2)</td>
  </tr>
</table>

**Note at (1)**: GCC's symbol mangling discards the access specifier of the method.
Therefore, we further mangle the return type of the method:
```py
def add_access_specifier(access: Literal['public', 'protected', 'private'],
                         retType: Type):
    return 'N' + encode_ns(access, retType.wholename()) + 'E'
```

**Note at (2)**: C symbols are mangled as if they are in a namespace named `extern`.
The `extern` is a keyword in C/C++, so no one will use it as a 
namespace name. This way we can avoid name collision between C symbols and C++ symbols.


# Implementation

## Source code Parsing

Done by clang frontend.

## Namespace Visitor

```py
# insert to table namespace
def insert_into_namespace(ns: str,
                          member: str,
                          type_: str) -> None: ...

for member in SomeNamespace:
    if isinstance(member, Function):
        insert_into_namespace(SomeNamespace.wholename(),
                              member.shortname(),
                              member.return_type.wholename())
    elif isinstance(member, Class) or isinstance(member, Namespace):
        # recursively visit it and its members.
        visit_class_or_namespace(member)
        insert_into_namespace(SomeNamespace.wholename(),
                              member.shortname(),
                              member.wholename())
    elif isinstance(member, DataMember):
        insert_into_namespace(SomeNamespace.wholename(),
                              member.shortname(),
                              member.type_.wholename())
```

## Class Visitor

```py
# insert to table class
def insert_into_class(cls: str,
                      member: str,
                      type_: str) -> None: ...

for member in SomeClass:
    if isinstance(member, Function) or isinstance(member, DataMember):
        insert_into_class(SomeClass.wholename(),
                          member.shortname(),
                          add_access_specifier(member.access,
                                               member.return_type))
    elif isinstance(member, Class):
        # recursively visit it and its members.
        visit_class_or_namespace(member)
        insert_into_class(SomeClass.wholename(),
                          member.shortname(),
                          member.wholename())
```
