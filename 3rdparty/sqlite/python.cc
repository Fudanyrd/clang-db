/**
 * Python binding built with Fudanyrd/cppbind
 */

#include <cppbind-sys.h>
#define this_package "sqlite" /* name of the python ext */

#include "sqlite_cxx.hh"
#define SQLite3Methods(X)                                                      \
  X(prepare, prepare, ::sqlite::SQLite3Stmt, ::cppbind::Str)                   \
  X(close, close, void)

#define SQLite3StmtMethods(X)                                                  \
  X(step, step, int)                                                           \
  X(bind, bind, int, ::cppbind::Str, PyObject *)                               \
  X(get_columns, get_columns, PyObject *, ::cppbind::Str)

namespace sqlite {

static int stmt_bind_impl(const SQLite3Stmt &stmt, char fmt,
                          ::cppbind::Object item, int idx) {
  int res;
  switch (fmt) {
  case 's': {
    ::cppbind::Str str_item(item);
    /*
     * C++ string is essentially a byte buffer,
     * so encode() is required.
     */
    std::string value = ::cppbind::stringify(str_item);
    res = ::sqlite::bind<const std::string &>(stmt, idx, value);
    break;
  }
  case 'b': {
    ::cppbind::Bytes bytes_item(item);
    const void *data = bytes_item.data();
    size_t size = bytes_item.size();
    res = ::sqlite::bind(stmt, idx, std::make_pair(data, size));
    break;
  }
  case 'i': {
    ::cppbind::Long value(item);
    int int_item = static_cast<long>(value);
    res = ::sqlite::bind(stmt, 0, int_item);
    break;
  }
  case 'l': {
    ::cppbind::Long value(item);
    auto int_item = static_cast<long long>(value);
    res = ::sqlite::bind(stmt, 0, int_item);
    break;
  }
  case 'd': {
    double double_item = ::cppbind::Float(item);
    res = ::sqlite::bind(stmt, 0, double_item);
    break;
  }
  case 'n': {
    res = sqlite3_bind_null(stmt.get(), 0);
    break;
  }
  default:
    return SQLITE_MISUSE;
  }
  return res;
}

template <typename ListLikeTy>
int stmt_bind(const SQLite3Stmt &stmt, ::cppbind::Str fmt, ListLikeTy value) {
  if (fmt.size() != value.size()) {
    return SQLITE_MISUSE;
  }

  const char *data = fmt.data();
  for (Py_ssize_t i = 0; i < fmt.size(); i++) {
    char c = data[i];
    auto item = value[i];
    int res = stmt_bind_impl(stmt, c, item, i + 1);
    if (res != SQLITE_OK) {
      return res;
    }
  }
  return SQLITE_OK;
}

int SQLite3Stmt::bind(::cppbind::Str fmt, PyObject *value) const {
  if (PyTuple_Check(value)) {
    return stmt_bind(*this, fmt, ::cppbind::Tuple{value});
  }
  if (PyList_Check(value)) {
    return stmt_bind(*this, fmt, ::cppbind::List{value});
  }
  /* unsupported type. */
  return SQLITE_MISUSE;
}

PyObject *SQLite3Stmt::get_columns(::cppbind::Str fmt) const {
  int column_count = fmt.size();
  auto *tuple = PyTuple_New(column_count);
  if (tuple == nullptr) {
    PyErr_SetString(PyExc_TypeError, "failed to create tuple");
    return nullptr;
  }
  const char *data = fmt.data();
  for (int i = 0; i < column_count; i++) {
    char c = data[i];
    PyObject *item = nullptr;
    switch (c) {
    case 's': {
      const char *str_item =
          reinterpret_cast<const char *>(sqlite3_column_text(get(), i));
      item = PyUnicode_FromString(str_item);
      break;
    }
    case 'b': {
      const void *data = sqlite3_column_blob(get(), i);
      int size = sqlite3_column_bytes(get(), i);
      item =
          PyBytes_FromStringAndSize(reinterpret_cast<const char *>(data), size);
      break;
    }
    case 'i': {
      int int_item = sqlite3_column_int(get(), i);
      item = PyLong_FromLong(int_item);
      break;
    }
    case 'l': {
      sqlite3_int64 int_item = sqlite3_column_int64(get(), i);
      item = PyLong_FromLong(int_item);
      break;
    }
    case 'd': {
      double double_item = sqlite3_column_double(get(), i);
      item = PyFloat_FromDouble(double_item);
      break;
    }
    case 'n': /* fall through */
    default:  /* silently return none as result. */
      Py_INCREF(Py_None);
      item = Py_None;
      break;
    }
    PyTuple_SET_ITEM(tuple, i, item);
  }

  return tuple;
}

SQLite3Stmt SQLite3::prepare(::cppbind::Str sql) const {
  std::string sql_str = ::cppbind::stringify(sql);
  return prepare(sql_str.data(), sql_str.size());
}

} /* namespace sqlite */

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
