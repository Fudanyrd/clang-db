#ifndef __STORAGE_H__
#define __STORAGE_H__ (1)

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <string_view>
#include <tuple>
#include <vector>

/* Sqlite3 CXX API */
#include <sqlite_cxx.hh>

#include "common.h"

namespace clang {
class TestHelper; /* Make our test helper a friend. */

namespace database _CLANGDB_VISIBILITY {

struct DatabaseInterface;

/**
 * The motivation for this class is that storing
 * a pointer and an extra bit will waste (at least)
 * one byte. Therefore, even for 32-bit systems,
 * the pointer's lower two bits shall be zero.
 */
struct QualPtr {
private:
  static constexpr uintptr_t PtrMask = uintptr_t(0x3);
  uintptr_t Ptr;

public:
  template <typename T>
  inline QualPtr(T *Pt) : Ptr(reinterpret_cast<uintptr_t>(Pt)) {
    static_assert((alignof(T) & PtrMask) == 0,
                  "Pointer's lower two bits must be zero");
  }

  QualPtr() : Ptr(0) {
    static_assert(sizeof(QualPtr) == sizeof(void *),
                  "QualPtr should be the same size as a pointer");
  }

  template <typename T> inline T *getPtr() const {
    static_assert((alignof(T) & PtrMask) == 0,
                  "Pointer's lower two bits must be zero");
    return reinterpret_cast<T *>(Ptr & (~PtrMask));
  }

  template <int Bit> bool get() const {
    static_assert(Bit == 0 || Bit == 1, "Bit must be 0 or 1");
    return (Ptr >> Bit) & 0x1;
  }

  template <int Bit> void set(bool Value = true) {
    static_assert(Bit == 0 || Bit == 1, "Bit must be 0 or 1");
    if (Value) {
      Ptr |= (uintptr_t(1) << Bit);
    } else {
      Ptr &= ~(uintptr_t(1) << Bit);
    }
  }
};

class alignas(void *) DatabaseInterface {
protected:
  SourceManager *SrcMgr;

public:
  void SetSourceManager(SourceManager &SM) { SrcMgr = &SM; }

  virtual ~DatabaseInterface() = default;

  virtual int TransactionBegin() = 0;
  virtual int Commit() = 0;

  /**
   * Zero value indicates success;
   * non-zero value indicates failure.
   */
  using DBExecResult = int;

  /**
   * Insert a record into table `class`.
   */
  virtual int InsertIntoClass(std::string_view ClassName, std::string_view Name,
                              std::string_view Type) = 0;

  /**
   * Insert a record into table `namespace`.
   */
  virtual int InsertIntoNamespace(std::string_view NamespaceName,
                                  std::string_view Child,
                                  std::string_view Type) = 0;

  virtual int InsertSymbol(std::string_view Name, std::string File,
                           int Line) = 0;

  int InsertSymbol(std::string_view Name, SourceLocation Loc) {
    if (!Loc.isValid()) {
      return 1; /* Invalid location */
    }
    PresumedLoc PLoc = SrcMgr->getPresumedLoc(Loc);
    if (PLoc.isInvalid()) {
      return 1; /* Invalid presumed location */
    }
    return InsertSymbol(Name, PLoc.getFilename(), PLoc.getLine());
  }
};

/**
 * Keep two tables in memory. Only used for testing purposes.
 */
class InMemoryDatabase : public DatabaseInterface {
private:
  std::vector<std::tuple<std::string, std::string, std::string>> Classes;
  std::vector<std::tuple<std::string, std::string, std::string>> Namespaces;
  std::vector<std::tuple<std::string, std::string, int>> Symbols;
  bool InTransaction = false;

  friend class ::clang::TestHelper;

public:
  InMemoryDatabase() = default;
  ~InMemoryDatabase() override = default;

  int TransactionBegin() override {
    InTransaction = true;
    return 0;
  }
  int Commit() override {
    InTransaction = false;
    return 0;
  }

  int InsertIntoClass(std::string_view ClassName, std::string_view Name,
                      std::string_view Type) override {
    clangdb_check_internal(InTransaction);
    Classes.emplace_back(ClassName, Name, Type);
    return 0;
  }

  int InsertIntoNamespace(std::string_view NamespaceName,
                          std::string_view Child,
                          std::string_view Type) override {
    clangdb_check_internal(InTransaction);
    Namespaces.emplace_back(NamespaceName, Child, Type);
    return 0;
  }

  const std::vector<std::tuple<std::string, std::string, std::string>> &
  GetClasses() const {
    return Classes;
  }
  const std::vector<std::tuple<std::string, std::string, std::string>> &
  GetNamespaces() const {
    return Namespaces;
  }

  int InsertSymbol(std::string_view Name, std::string File, int Line) override {
    Symbols.emplace_back(Name, File, Line);
    return 0;
  }
};

class SqliteDatabase : public DatabaseInterface {

/* Name of tables in the database. */
#define ClangDBTableClass "class"
#define ClangDBTableNamespace "namespace"
#define ClangDBTableSymbol "symbol"

private:
  ::sqlite::SQLite3 DB;

  void CreateTableAndIndex();

public:
  operator bool() const { return (bool)DB; }
  SqliteDatabase() : DB(0) { CreateTableAndIndex(); }
  SqliteDatabase(const char *Filename) : DB(Filename) { CreateTableAndIndex(); }
  ~SqliteDatabase() override = default;

  int TransactionBegin() override;
  int Commit() override;

  int InsertIntoClass(std::string_view ClassName, std::string_view Name,
                      std::string_view Type) override;
  int InsertIntoNamespace(std::string_view NamespaceName,
                          std::string_view Child,
                          std::string_view Type) override;
  int InsertSymbol(std::string_view Name, std::string File, int Line) override;
};

} // namespace database _CLANGDB_VISIBILITY
} /* namespace clang */

#endif /* __STORAGE_H__ */
