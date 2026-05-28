#ifndef __SQLITE_CXXAPI_H__
#define __SQLITE_CXXAPI_H__ (1)

#if !defined _SQLITE3_VISIBILITY
#define _SQLITE3_VISIBILITY "default"
#endif /* _SQLITE3_VISIBILITY */

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <sys/fcntl.h>
#include <utility>
#include <vector>

#if defined _SQLITE_HAS_PYTHON
#include <Python.h>
#endif /* _SQLITE_HAS_PYTHON */

#include "sqlite3.h"

/* Python API: forward declaration. */
extern "C" typedef struct _object PyObject;

/* cppbind classes declaration. */
namespace cppbind {
struct Bytes;
struct Tuple;
struct List;
struct Object;
struct Dict;
struct Str;
} /* namespace cppbind */

namespace sqlite __attribute__((visibility(_SQLITE3_VISIBILITY))) {

  struct SQLite3Stmt;

  /**
   * Bind a value to a prepared statement.
   * The index is 1-based, as in the SQLite C API.
   */
  template <typename T>
  inline int bind(const SQLite3Stmt &stmt, int index, T value);

  /**
   * Get the column value of the current row of a prepared statement.
   * The index is 0-based, as in the SQLite C API.
   */
  template <typename T>
  inline int get_column(SQLite3Stmt & stmt, int index, T *out);

  /**
   * The {@link Binder} class handles binding C++ values
   * to arguments to SQL statements, in ascending order of argument index.
   *
   * Note that the index is 1-based, therefore user of this
   * class should set argument `Idx` to 1.
   *
   * Its static method `bind` handles the binding, which returns
   * `SQLITE_OK` on success.
   */
  template <int Idx, typename... Args> struct Binder;

  template <int Idx, typename Head, typename... RestArgs>
  struct Binder<Idx, Head, RestArgs...> {
    static int bind(const SQLite3Stmt &stmt, Head head, RestArgs... rest) {
      int res = ::sqlite::bind<Head>(stmt, Idx, head);
      if (res != SQLITE_OK) {
        return res;
      }
      return Binder<Idx + 1, RestArgs...>::bind(stmt, rest...);
    }
  };

  template <int Idx, typename Head> struct Binder<Idx, Head> {
    static int bind(const SQLite3Stmt &stmt, Head head) {
      return ::sqlite::bind<Head>(stmt, Idx, head);
    }
  };

  /**
   * A result from SQL query execution. Its method `get_one`
   * fills the provided reference of the value in the tuple result,
   * which returns `SQLITE_OK` on success.
   *
   * Note: the left-most result has the index 0.
   */
  template <int Idx, typename... Args> struct Row;

  template <int Idx, typename Head> struct Row<Idx, Head> {
    Head &head; /* One column of execution result. */
    Row(Head &head) : head(head) {}

    int get_one(SQLite3Stmt &stmt) {
      return ::sqlite::get_column(stmt, Idx, &head);
    }
  };

  template <int Idx, typename Head, typename... RestArgs>
  struct Row<Idx, Head, RestArgs...> : public Row<Idx + 1, RestArgs...> {
  private:
    using ExtendedTy = Row<Idx + 1, RestArgs...>;

  public:
    Head &head; /* One column of execution result. */
    Row(Head &head, RestArgs &...rest) : ExtendedTy(rest...), head(head) {}

    /**
     * Get one result.
     */
    int get_one(SQLite3Stmt &stmt) {
      int res = ::sqlite::get_column(stmt, Idx, &head);
      if (res != SQLITE_OK) {
        return res;
      }
      return ExtendedTy::get_one(stmt);
    }
  };

  struct SQLite3Stmt {
  private:
    sqlite3_stmt *stmt;

  public:
    /**
     * BUG prior to commit 0d08f345: possibly use-after-free.
     *
     * This tries to resolve the bug, without copying the string/blob data,
     * by keeping the reference of these objects alive until the statement is
     * finalized.
     *
     * If cppbind is present, vector&lt;cppbind::Bytes&gt; should work,
     * but for compatibility, we store object pointers.
     */
    mutable std::vector<PyObject *> bound_objects;

  public:
    ~SQLite3Stmt() {
      /* Ignore return value for now. */
      (void)sqlite3_finalize(stmt);

      /* Check whether cppbind package is present, via cmake provided define. */
#if defined _SQLITE_HAS_PYTHON
      for (auto *object : bound_objects) {
        /* Release the reference of these objects. */
        Py_DECREF(object);
      }
#endif /* _SQLITE_HAS_PYTHON */
    }

    SQLite3Stmt() : stmt(nullptr) {}
    explicit SQLite3Stmt(sqlite3_stmt *s) : stmt(s) {}

    sqlite3_stmt *get() const { return stmt; }
    operator bool() const { return stmt != nullptr; }

    SQLite3Stmt(const SQLite3Stmt &) = delete;
    SQLite3Stmt &operator=(const SQLite3Stmt &) = delete;

    SQLite3Stmt(SQLite3Stmt &&other) noexcept : stmt(other.stmt) {
      other.stmt = nullptr;
    }
    SQLite3Stmt &operator=(SQLite3Stmt &&other) noexcept {
      if (this != &other) {
        sqlite3_finalize(stmt);
        stmt = other.stmt;
        other.stmt = nullptr;
      }
      return *this;
    }

    int step(void) const { return sqlite3_step(stmt); }

    int bind(int index, const char *value, size_t len) const {
      return sqlite3_bind_text(stmt, index, value, len, SQLITE_STATIC);
    }
    int bind(int index, std::string_view value) const {
      return sqlite3_bind_text(stmt, index, value.data(), value.size(),
                               SQLITE_STATIC);
    }

    /**
     * Only used when `SQLITE_BUILD_PYTHON` is enabled.
     *
     * @param fmt: a string consists of only s(str), b(blob),
     * i(int), l(int64), d(double), n(null).
     * @param value: a tuple or list of values to be bound, which should
     * have the same number of items as the length of `fmt`.
     */
    int bind(::cppbind::Str fmt, PyObject *value) const;

    /**
     * Only used when `SQLITE_BUILD_PYTHON` is enabled.
     *
     * @param fmt: a string consists of only s(str), b(blob),
     * i(int), l(int64), d(double), n(null).
     * @return a tuple of column values of the current row,
     * nullptr on error.
     */
    PyObject *get_columns(::cppbind::Str fmt) const;
  };

  struct SQLite3 {
  private:
    sqlite3 *db;

  public:
    ~SQLite3() {
      /* Ignore return value for now. */
      (void)sqlite3_close(db);
    }
    void close() {
      (void)sqlite3_close(db);
      db = nullptr;
    }

    SQLite3() : db(nullptr) {}
    SQLite3(const char *filename,
            int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
      if (sqlite3_open_v2(filename, &db, flags, nullptr) != SQLITE_OK) {
        db = nullptr;
      }
    }

    /**
     * Opens an in-memory database. This is useful for testing.
     */
    SQLite3(int magic,
            int rest_flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) {
      if (sqlite3_open_v2(nullptr, &db, SQLITE_OPEN_MEMORY | rest_flags,
                          nullptr) != SQLITE_OK) {
        abort(); /* This should never happen. */
      }
    }

    sqlite3 *get() const { return db; }
    operator bool() const { return db != nullptr; }

    SQLite3(const SQLite3 &) = delete;
    SQLite3 &operator=(const SQLite3 &) = delete;

    SQLite3(SQLite3 &&other) noexcept : db(other.db) { other.db = nullptr; }
    SQLite3 &operator=(SQLite3 &&other) noexcept {
      if (this != &other) {
        sqlite3_close(db);
        db = other.db;
        other.db = nullptr;
      }
      return *this;
    }

    SQLite3Stmt prepare(const char *sql) const {
      return prepare(sql, strlen(sql));
    }

    SQLite3Stmt prepare(const char *sql, size_t len) const {
      sqlite3_stmt *stmt = nullptr;
      if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, nullptr) !=
          SQLITE_OK) {
        return SQLite3Stmt(nullptr);
      }
      return SQLite3Stmt(stmt);
    }

    SQLite3Stmt prepare(::cppbind::Str sql) const;

    int errcode() const { return sqlite3_errcode(db); }
    const char *err_message() const { return sqlite3_errmsg(db); }
  };

  template <>
  inline int bind(const SQLite3Stmt &stmt, int index, const char *value) {
    return stmt.bind(index, value, strlen(value));
  }
  template <>
  inline int bind(const SQLite3Stmt &stmt, int index, std::string_view value) {
    return stmt.bind(index, value.data(), value.size());
  }

  template <>
  inline int bind<const std::string &>(const SQLite3Stmt &stmt, int index,
                                       const std::string &value) {
    return stmt.bind(index, value.data(), value.size());
  }

  template <> inline int bind(const SQLite3Stmt &stmt, int index, int value) {
    return sqlite3_bind_int(stmt.get(), index, value);
  }

  template <>
  inline int bind(const SQLite3Stmt &stmt, int index, sqlite3_int64 value) {
    return sqlite3_bind_int64(stmt.get(), index, value);
  }

  template <>
  inline int bind(const SQLite3Stmt &stmt, int index, std::nullptr_t) {
    return sqlite3_bind_null(stmt.get(), index);
  }

  template <>
  inline int bind(const SQLite3Stmt &stmt, int index, double value) {
    return sqlite3_bind_double(stmt.get(), index, value);
  }
  template <> inline int bind(const SQLite3Stmt &stmt, int index, float value) {
    return sqlite3_bind_double(stmt.get(), index, static_cast<double>(value));
  }

  template <>
  inline int bind(const SQLite3Stmt &stmt, int index,
                  std::pair<const void *, size_t> value) {
    return sqlite3_bind_blob(stmt.get(), index, value.first, value.second,
                             SQLITE_STATIC);
  }

  template <> inline int get_column(SQLite3Stmt & stmt, int index, int *out) {
    *out = sqlite3_column_int(stmt.get(), index);
    return SQLITE_OK;
  }

  template <>
  inline int get_column(SQLite3Stmt & stmt, int index, sqlite3_int64 *out) {
    *out = sqlite3_column_int64(stmt.get(), index);
    return SQLITE_OK;
  }

  template <>
  inline int get_column(SQLite3Stmt & stmt, int index, double *out) {
    *out = sqlite3_column_double(stmt.get(), index);
    return SQLITE_OK;
  }

  /**
   * Get a blob column.
   */
  template <>
  inline int get_column<std::pair<const void *, size_t>>(
      SQLite3Stmt & stmt, int index, std::pair<const void *, size_t> *out) {
    const void *data = sqlite3_column_blob(stmt.get(), index);
    int size = sqlite3_column_bytes(stmt.get(), index);
    *out = {data, static_cast<size_t>(size)};
    return SQLITE_OK;
  }

  /**
   * Get a text column.
   */
  template <>
  inline int get_column<std::string>(SQLite3Stmt & stmt, int index,
                                     std::string *out) {
    const char *text =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), index));
    if (text == nullptr) {
      text = ""; /* SQLite Specified. */
    }
    int size = sqlite3_column_bytes(stmt.get(), index);
    *out = std::string(text, size);
    return SQLITE_OK;
  }

  template <>
  inline int get_column<std::pair<const char *, size_t>>(
      SQLite3Stmt & stmt, int index, std::pair<const char *, size_t> *out) {
    const char *text =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), index));
    if (text == nullptr) {
      text = ""; /* SQLite Specified. */
    }
    int size = sqlite3_column_bytes(stmt.get(), index);
    *out = {text, static_cast<size_t>(size)};
    return SQLITE_OK;
  }

} /* namespace sqlite */

#endif /* __SQLITE_CXXAPI_H__ */
