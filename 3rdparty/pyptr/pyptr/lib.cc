#include <cppbind-sys.h>

#define this_package "pyptr" /* name of the python ext */

/**
 * A class that stores pointer to a python object, without owning it.
 * It is useful for creating weak references.
 */
struct PyPointer {
  PyPointer(PyObject *ptr) : ptr(ptr) {}
  PyPointer() : ptr(Py_None) {}
  ~PyPointer() = default;

  PyObject *ptr;
};

#define py_ptr_methods(X)

namespace cppbind {
cpp_class_wrapper(PyPointer, py_ptr_methods);
}

method_wrapper_static_members_declare(cppbind::MethodTableEntry::method_t);
type_static_members(cppbind::CppObject<PyPointer>);

static PyObject *refcnt(PyObject *self, PyObject *const *args,
                        Py_ssize_t nargs) {
  if (nargs != 1) {
    PyErr_SetString(PyExc_TypeError, "refcnt takes exactly one argument");
    return nullptr;
  }
  long value = args[0]->ob_refcnt;
  return cppbind::Long(value).object().unwrap();
}

static int rest_init(cppbind::Module &modobj) {
  MethodWrapper_init(this_package, cppbind::MethodTableEntry::method_t);
  cpp_type_init(this_package, PyPointer, "PyPointer", py_ptr_methods);

  PyTypeObject *pypointer_type = cpp_get_type_object(PyPointer);
  pypointer_type->tp_init = [](PyObject *self, PyObject *raw_args,
                               PyObject *kwargs) -> int {
    cppbind::Tuple args = cppbind::Tuple::from_args(raw_args);
    auto *payload = cppbind::CppObject<PyPointer>::get_payload(self);
    if (args.size() == 0) {
      new (payload) PyPointer();
    } else if (args.size() == 1) {
      PyObject *ptr = args[0].ptr;
      new (payload) PyPointer(ptr);
    } else {
      PyErr_SetString(PyExc_TypeError, "PyPointer takes at most one argument");
      return -1;
    }
    return 0;
  };

  pypointer_type->tp_call = [](PyObject *self, PyObject *raw_args,
                               PyObject *kwargs) -> PyObject * {
    cppbind::Tuple args = cppbind::Tuple::from_args(raw_args);
    auto *payload = cppbind::CppObject<PyPointer>::get_payload(self);
    auto *ret = payload->ptr;
    Py_INCREF(ret); /* give the caller a reference. */
    return ret;
  };
  pypointer_type->tp_alloc = PyType_GenericAlloc;
  pypointer_type->tp_new = PyType_GenericNew;
  PyType_Ready(pypointer_type);

  modobj.add_object("PyPointer", reinterpret_cast<PyObject *>(pypointer_type));
  return 0;
}

gen_modinit_fn_from_fns(
    /* name */ pyptr, &rest_init, nullptr, nullptr, nullptr,
    {"refcnt", (PyCFunction)refcnt, METH_FASTCALL,
     "Get the reference count of a python object."});
