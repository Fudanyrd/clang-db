#ifndef __SQLITE_CXXAPI_H__
#define __SQLITE_CXXAPI_H__ (1)

#if !defined _SQLITE3_VISIBILITY
#define _SQLITE3_VISIBILITY "default"
#endif /* _SQLITE3_VISIBILITY */

#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/fcntl.h>

#include "sqlite3.h"

namespace sqlite __attribute__((visibility(_SQLITE3_VISIBILITY))) {

  struct SQLite3Stmt {
  private:
    sqlite3_stmt *stmt;

  public:
    ~SQLite3Stmt() {
      /* Ignore return value for now. */
      (void)sqlite3_finalize(stmt);
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
  };

  struct SQLite3 {
  private:
    sqlite3 *db;

  public:
    ~SQLite3() {
      /* Ignore return value for now. */
      (void)sqlite3_close(db);
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

    int errcode() const { return sqlite3_errcode(db); }
    const char *err_message() const { return sqlite3_errmsg(db); }
  };

} /* namespace sqlite */

#endif /* __SQLITE_CXXAPI_H__ */
