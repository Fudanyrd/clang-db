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

template <typename... ColumnTypes> struct alignas(void *) IteratorBase {
  virtual ~IteratorBase() = default;

  /**
   * @return number of row(tuple) produced, or negative on error.
   *
   * Example of collecting all output tuples:
   * <blockquote><pre>
   *   IteratorBase<std::string, int> *It;
   *   int Result;
   *   while ((Result = It->next(OutStr, OutInt)) > 0) {
   *     // Process OutStr and OutInt.
   *   }
   *  if (Result < 0) {
   *    // handle specific database error.
   *  }
   * </pre></blockquote>
   */
  virtual int next(ColumnTypes &...Out) = 0;

  /**
   * @return maximum size of iterator types extending this.
   *
   * With this, one can avoid allocating the iterator on the head,
   * for example:
   * <blockquote><pre>
   *   char IteratorBuf[ IteratorBase<std::string, int>::AllocSize() ];
   *   new (IteratorBuf) IteratorDerived(...);
   * </pre></blockquote>
   */
  static constexpr size_t AllocSize();
};

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

  friend struct IteratorBase<std::string, int>;
  friend struct IteratorBase<std::string, std::string>;
  friend struct IteratorBase<std::string>;

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

  /* Querying the Database */

  /**
   * Equivalent SQL:
   * <blockquote><pre>
   * SELECT file, line FROM class
   * where name = $Name;
   * </pre></blockquote>
   *
   * @param Name: query key.
   * @param Buffer: A buffer for the iterator to use. The caller is responsible
   * for ensuring the buffer is large enough for the iterator. The size can be
   * obtained by `IteratorBase::AllocSize()`. If it is null, the returned
   * iterator will be allocated by `new`, and the caller is responsible for
   * `delete`-ing it.
   */
  virtual IteratorBase<std::string, int> *QuerySymbol(std::string_view Name,
                                                      char *Buffer) = 0;

  /**
   * Equivalent SQL:
   * <blockquote><pre>
   * SELECT member, type FROM class
   * where name = $ClassName;
   * </pre></blockquote>
   *
   * @param ClassName
   * @param Buffer A buffer for the iterator to use. The caller is responsible
   * for ensuring the buffer is large enough for the iterator. The size can be
   * obtained by `IteratorBase::AllocSize()`. If it is null, the returned
   * iterator will be allocated by `new`, and the caller is responsible for
   * `delete`-ing it.
   */
  virtual IteratorBase<std::string, std::string> *
  QueryClass(std::string_view ClassName, char *Buffer) = 0;

  /**
   * Equivalent SQL:
   * <blockquote><pre>
   * SELECT type FROM class
   * where (name = $ClassName) and (member <> $Member);
   * </pre></blockquote>
   *
   * @param ClassName
   * @param Member
   * @param Buffer A buffer for the iterator to use. The caller is responsible
   * for ensuring the buffer is large enough for the iterator. The size can be
   * obtained by `IteratorBase::AllocSize()`. If it is null, the returned
   * iterator will be allocated by `new`, and the caller is responsible for
   * `delete`-ing it.
   */
  virtual IteratorBase<std::string> *QueryClass(std::string_view ClassName,
                                                std::string_view Member,
                                                char *Buffer) = 0;

  /**
   * Equivalent SQL:
   * <blockquote><pre>
   * SELECT member, type FROM namespace
   * where name = $NsName;
   * </pre></blockquote>
   *
   * @param NsName
   * @param Buffer A buffer for the iterator to use. The caller is responsible
   * for ensuring the buffer is large enough for the iterator. The size can be
   * obtained by `IteratorBase::AllocSize()`. If it is null, the returned
   * iterator will be allocated by `new`, and the caller is responsible for
   * `delete`-ing it.
   */
  virtual IteratorBase<std::string, std::string> *
  QueryNamespace(std::string_view NsName, char *Buffer) = 0;

  /**
   * Equivalent SQL:
   * <blockquote><pre>
   * SELECT type FROM namespace
   * where name = $NsName and member = $Member;
   * </pre></blockquote>
   *
   * @param NsName
   * @param Member
   * @param Buffer A buffer for the iterator to use. The caller is responsible
   * for ensuring the buffer is large enough for the iterator. The size can be
   * obtained by `IteratorBase::AllocSize()`. If it is null, the returned
   * iterator will be allocated by `new`, and the caller is responsible for
   * `delete`-ing it.
   */
  virtual IteratorBase<std::string> *QueryNamespace(std::string_view NsName,
                                                    std::string_view Member,
                                                    char *Buffer) = 0;
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

  template <typename ThirdParmType>
  struct IteratorOneKey : public IteratorBase<std::string, ThirdParmType> {
  private:
    using Tuple = std::tuple<std::string, std::string, ThirdParmType>;
    typename std::vector<Tuple>::size_type Index = 0;
    const std::vector<Tuple> &Data;
    std::string_view Key;

    bool Predicate(const Tuple &T) const { return std::get<0>(T) == Key; }

  public:
    IteratorOneKey(const std::vector<Tuple> &Data, std::string_view Key)
        : Data(Data), Key(Key) {}

    int next(std::string &OutName, ThirdParmType &OutType) override;
  };

  template <typename ThirdParmType>
  struct IteratorTwoKeys : public IteratorBase<ThirdParmType> {
  private:
    using Tuple = std::tuple<std::string, std::string, ThirdParmType>;
    typename std::vector<Tuple>::size_type Index = 0;
    const std::vector<Tuple> &Data;
    std::string_view Key1;
    std::string_view Key2;

    bool Predicate(const Tuple &T) const {
      return std::get<0>(T) == Key1 && std::get<1>(T) == Key2;
    }

  public:
    IteratorTwoKeys(const std::vector<Tuple> &Data, std::string_view Key1,
                    std::string_view Key2)
        : Data(Data), Key1(Key1), Key2(Key2) {}

    int next(ThirdParmType &OutType) override;
  };

  IteratorBase<std::string, int> *QuerySymbol(std::string_view Name,
                                              char *Buffer) override {
    if (Buffer) {
      return new (Buffer) IteratorOneKey<int>(Symbols, Name);
    }
    return new IteratorOneKey<int>(Symbols, Name);
  }

  IteratorBase<std::string, std::string> *QueryClass(std::string_view ClassName,
                                                     char *Buffer) override {
    if (Buffer) {
      return new (Buffer) IteratorOneKey<std::string>(Classes, ClassName);
    }
    return new IteratorOneKey<std::string>(Classes, ClassName);
  }

  IteratorBase<std::string> *QueryClass(std::string_view ClassName,
                                        std::string_view Member,
                                        char *Buffer) override {
    if (Buffer) {
      return new (Buffer)
          IteratorTwoKeys<std::string>(Classes, ClassName, Member);
    }
    return new IteratorTwoKeys<std::string>(Classes, ClassName, Member);
  }

  IteratorBase<std::string, std::string> *
  QueryNamespace(std::string_view NsName, char *Buffer) override {
    if (Buffer) {
      return new (Buffer) IteratorOneKey<std::string>(Namespaces, NsName);
    }
    return new IteratorOneKey<std::string>(Namespaces, NsName);
  }

  IteratorBase<std::string> *QueryNamespace(std::string_view NsName,
                                            std::string_view Member,
                                            char *Buffer) override {
    if (Buffer) {
      return new (Buffer)
          IteratorTwoKeys<std::string>(Namespaces, NsName, Member);
    }
    return new IteratorTwoKeys<std::string>(Namespaces, NsName, Member);
  }

  static constexpr size_t IteratorSize() {
    return std::max(
        {sizeof(IteratorOneKey<int>), sizeof(IteratorOneKey<std::string>),
         sizeof(IteratorTwoKeys<std::string>), sizeof(IteratorTwoKeys<int>)});
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
  template <typename... ColumnTypes>
  struct Iterator : public IteratorBase<ColumnTypes...> {
  private:
    ::sqlite::SQLite3Stmt Stmt;

  public:
    Iterator(::sqlite::SQLite3Stmt &&Stmt) : Stmt(std::move(Stmt)) {}

    int next(ColumnTypes &...Out) override {
      ::sqlite::Row<0, ColumnTypes...> Row(Out...);
      int StepRes = Stmt.step();
      if (StepRes == SQLITE_ROW) {
        /* Ignore returned data-type code. */
        (void)Row.get_one(Stmt);
        return 1;
      }
      if (StepRes == SQLITE_DONE) {
        return 0; /* No more data. */
      }
      return -1; /* Error. */
    }
  };

  IteratorBase<std::string, int> *QuerySymbol(std::string_view Name,
                                              char *Buffer) override;

  IteratorBase<std::string, std::string> *QueryClass(std::string_view ClassName,
                                                     char *Buffer) override;

  IteratorBase<std::string> *QueryClass(std::string_view ClassName,
                                        std::string_view Member,
                                        char *Buffer) override;

  IteratorBase<std::string, std::string> *
  QueryNamespace(std::string_view NsName, char *Buffer) override;

  IteratorBase<std::string> *QueryNamespace(std::string_view NsName,
                                            std::string_view Member,
                                            char *Buffer) override;

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

  ::sqlite::SQLite3 &GetDB() { return DB; }

  static constexpr size_t IteratorSize() {
    return sizeof(Iterator<std::string, int>);
  }
};

template <typename... ColumnTypes>
constexpr size_t IteratorBase<ColumnTypes...>::AllocSize() {
  return std::max(InMemoryDatabase::IteratorSize(),
                  SqliteDatabase::IteratorSize());
}

} // namespace database _CLANGDB_VISIBILITY
} /* namespace clang */

#endif /* __STORAGE_H__ */
