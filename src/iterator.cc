#include "common.h"
#include "storage.h"

/**
 * Iterators
 */
namespace clang {
namespace database _CLANGDB_VISIBILITY {

template <typename ThirdParmType>
int InMemoryDatabase::IteratorOneKey<ThirdParmType>::next(
    std::string &OutName, ThirdParmType &OutType) {
  while (Index < Data.size()) {
    const Tuple &T = Data[Index++];
    if (Predicate(T)) {
      OutName = std::get<1>(T);
      OutType = std::get<2>(T);
      return 1;
    }
  }
  return 0; /* No more data. */
}

/* Instantiate. */
template int InMemoryDatabase::IteratorOneKey<int>::next(std::string &OutName,
                                                         int &OutType);
template int
InMemoryDatabase::IteratorOneKey<std::string>::next(std::string &OutName,
                                                    std::string &OutType);

template <typename ThirdParmType>
int InMemoryDatabase::IteratorTwoKeys<ThirdParmType>::next(
    ThirdParmType &OutType) {
  while (Index < Data.size()) {
    const Tuple &T = Data[Index++];
    if (Predicate(T)) {
      OutType = std::get<2>(T);
      return 1;
    }
  }
  return 0; /* No more data. */
}

template int InMemoryDatabase::IteratorTwoKeys<int>::next(int &OutType);
template int
InMemoryDatabase::IteratorTwoKeys<std::string>::next(std::string &OutType);

/**
 * @param SQLStmt: (string-literal) SQL statement to be executed;
 * @param BinderType: type of binder to bind parameters to the SQL statement;
 * @param IteratorType: type of iterator to produce query results.
 * @param __VA_ARGS__: values to be bound to the SQL statement.
 */
#define SqliteDBQueryImpl(SQLStmt, BinderType, IteratorType, ...)              \
  static constexpr char STMT[] = SQLStmt;                                      \
  ::sqlite::SQLite3Stmt SqliteStmt = DB.prepare(STMT, sizeof(STMT) - 1);       \
  if (!SqliteStmt) {                                                           \
    return nullptr; /* Failed to prepare statement. */                         \
  }                                                                            \
  int Result = BinderType::bind(SqliteStmt, ##__VA_ARGS__);                    \
  if (Result != SQLITE_OK) {                                                   \
    return nullptr; /* Failed to bind parameters. */                           \
  }                                                                            \
  if (Buffer) {                                                                \
    return new (Buffer) IteratorType(std::move(SqliteStmt));                   \
  }                                                                            \
  return new IteratorType(std::move(SqliteStmt))

IteratorBase<std::string, int> *
SqliteDatabase::QuerySymbol(std::string_view Name, char *Buffer) {

  using Binder = ::sqlite::Binder<1, std::string_view>;
  using IteratorType = Iterator<std::string, int>;

  SqliteDBQueryImpl(
      ("SELECT file, line FROM " ClangDBTableSymbol " WHERE (name = ?);"),
      Binder, IteratorType, Name);
}

IteratorBase<std::string, std::string> *
SqliteDatabase::QueryClass(std::string_view Name, char *Buffer) {
  using Binder = ::sqlite::Binder<1, std::string_view>;
  using IteratorType = Iterator<std::string, std::string>;
  SqliteDBQueryImpl(
      ("SELECT member, type FROM " ClangDBTableClass " WHERE (name = ?);"),
      Binder, IteratorType, Name);
}

IteratorBase<std::string> *
SqliteDatabase::QueryClass(std::string_view ClassName, std::string_view Member,
                           char *Buffer) {
  using Binder = ::sqlite::Binder<1, std::string_view, std::string_view>;
  using IteratorType = Iterator<std::string>;
  SqliteDBQueryImpl(("SELECT type FROM " ClangDBTableClass
                     " WHERE (name = ?) AND (member = ?);"),
                    Binder, IteratorType, ClassName, Member);
}

IteratorBase<std::string, std::string> *
SqliteDatabase::QueryNamespace(std::string_view NsName, char *Buffer) {
  using Binder = ::sqlite::Binder<1, std::string_view>;
  using IteratorType = Iterator<std::string, std::string>;
  SqliteDBQueryImpl(
      ("SELECT member, type FROM " ClangDBTableNamespace " WHERE (name = ?);"),
      Binder, IteratorType, NsName);
}

IteratorBase<std::string> *
SqliteDatabase::QueryNamespace(std::string_view NsName, std::string_view Member,
                               char *Buffer) {
  using Binder = ::sqlite::Binder<1, std::string_view, std::string_view>;
  using IteratorType = Iterator<std::string>;

  SqliteDBQueryImpl(("SELECT type FROM " ClangDBTableNamespace
                     " WHERE (name = ?) AND (member = ?);"),
                    Binder, IteratorType, NsName, Member);
}

#undef SqliteDBQueryImpl

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
