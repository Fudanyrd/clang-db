/**
 * Python binding built with Fudanyrd/cppbind
 */

#include <cppbind-sys.h>
#define this_package "sqlite" /* name of the python ext */

#include "sqlite_cxx.hh"
#define SQLite3Methods(X)                                                      \
  X(prepare, prepare, ::sqlite::SQLite3Stmt, const char *)                     \
  X(close, close, void)

#define SQLite3StmtMethods(X) X(step, step, int)

namespace cppbind {

/* Declarations. */
template <> struct CppObject<sqlite::SQLite3Stmt>;
template <> struct CppObject<sqlite::SQLite3>;
type_static_members_declare(CppObject<sqlite::SQLite3Stmt>);
type_static_members_declare(CppObject<sqlite::SQLite3>);

template <>
Object into<::sqlite::SQLite3Stmt &&>(::sqlite::SQLite3Stmt &&value);
template <> Object into<::sqlite::SQLite3 &&>(::sqlite::SQLite3 &&value);

cpp_class_wrapper(sqlite::SQLite3, SQLite3Methods);
cpp_class_wrapper(sqlite::SQLite3Stmt, SQLite3StmtMethods);

template <> Object into<::sqlite::SQLite3Stmt>(::sqlite::SQLite3Stmt value) {
  PyObject *obj =
      PyObject_New(PyObject, Type<CppObject<::sqlite::SQLite3Stmt>>::instance);
  if (obj == nullptr) {
    return Object{nullptr};
  }
  new (obj + 1)::sqlite::SQLite3Stmt(std::move(value));
  return Object{obj};
}

template <>
Object into<::sqlite::SQLite3Stmt &&>(::sqlite::SQLite3Stmt &&value) {
  PyObject *obj =
      PyObject_New(PyObject, Type<CppObject<::sqlite::SQLite3Stmt>>::instance);
  if (obj == nullptr) {
    return Object{nullptr};
  }
  new (obj + 1)::sqlite::SQLite3Stmt(std::move(value));
  return Object{obj};
}

template <> Object into<::sqlite::SQLite3 *>(::sqlite::SQLite3 *value) {
  PyObject *obj =
      PyObject_New(PyObject, Type<CppObject<::sqlite::SQLite3>>::instance);
  if (obj == nullptr) {
    return Object{nullptr};
  }
  new (obj + 1)::sqlite::SQLite3(std::move(*value));
  return Object{obj};
}

} /* namespace cppbind */

namespace sqlite {

/**
 * In python, it is named (sqlite.)open.
 *
 * :param: filename: the name of the database file.
 * :param: ro (bool): open read-only, default False.
 */
extern "C" PyObject *sqlite3py_open(PyObject *self, PyObject *const *args,
                                    Py_ssize_t nargs) {
  if (nargs < 1 || nargs > 2) {
    PyErr_SetString(PyExc_TypeError, "open takes 1 or 2 arguments");
    return nullptr;
  }
  bool ro = args[1] == Py_True;
  std::string filename = ::cppbind::stringify(args[0]);
  int flags =
      (ro ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
  PyObject *obj = PyObject_New(
      PyObject,
      ::cppbind::Type<::cppbind::CppObject<::sqlite::SQLite3>>::instance);
  if (obj == nullptr) {
    PyErr_SetString(PyExc_TypeError, "failed to create Python object");
    return nullptr;
  }

  auto *db = reinterpret_cast<SQLite3 *>(obj + 1);
  new (db) SQLite3(filename.c_str(), flags);
  if (!(*db)) {
    PyErr_SetString(PyExc_TypeError, "failed to open database");
    return nullptr;
  }
  return obj;
}

extern "C" PyObject *
sqlite3py_open_memory(PyObject *self, PyObject *const *args, Py_ssize_t nargs) {
  if (nargs > 1) {
    PyErr_SetString(PyExc_TypeError, "open_memory takes no arguments");
    return nullptr;
  }
  PyObject *obj = PyObject_New(
      PyObject,
      ::cppbind::Type<::cppbind::CppObject<::sqlite::SQLite3>>::instance);
  if (obj == nullptr) {
    PyErr_SetString(PyExc_TypeError, "failed to create Python object");
    return nullptr;
  }

  auto *db = reinterpret_cast<SQLite3 *>(obj + 1);
  new (db) SQLite3(0);
  if (!(*db)) {
    PyErr_SetString(PyExc_TypeError, "failed to open in-memory database");
    return nullptr;
  }
  return obj;
}

/**
 * Todo: statement binding and `get_column` method.
 */

} /* namespace sqlite */

method_wrapper_static_members_declare(cppbind::MethodTableEntry::method_t);
type_static_members(cppbind::CppObject<sqlite::SQLite3Stmt>);
type_static_members(cppbind::CppObject<sqlite::SQLite3>);

static int rest_init() {
  MethodWrapper_init(this_package, cppbind::MethodTableEntry::method_t);
  cpp_type_init(this_package, sqlite::SQLite3Stmt, "Stmt", SQLite3StmtMethods);
  cpp_type_init(this_package, sqlite::SQLite3, "Database", SQLite3Methods);
  return 0;
}

gen_modinit_fn_from_fns(
    /* name */ sqlite, &rest_init, nullptr, nullptr, nullptr,
    {"open", (PyCFunction)::sqlite::sqlite3py_open, METH_FASTCALL,
     "Open a database file."},
    {"open_memory", (PyCFunction)::sqlite::sqlite3py_open_memory, METH_FASTCALL,
     "Open an in-memory database."});

#undef SQLite3Methods
#undef SQLite3StmtMethods
