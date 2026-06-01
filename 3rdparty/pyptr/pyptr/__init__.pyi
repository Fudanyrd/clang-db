"""
PyPtr: another weakref implementation for Python.
"""

def refcnt(obj: object) -> int:
    """
    Get the reference count of an object. It is only used
    in debugging and testing.
    """
    ...

class PyPointer():
    """
    A PyPointer is a pointer to a Python object. It is similar to a weakref, but it does not
    support callbacks and it does not keep the object alive. It is also faster than a weakref.
    """

    def __init__(self, obj: object) -> None: ...
    def __call__(self) -> object: ...
