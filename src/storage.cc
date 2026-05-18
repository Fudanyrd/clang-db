#include <clangdb.h>

namespace clang {
namespace database _CLANGDB_VISIBILITY {

int SqliteDatabase::TransactionBegin() {
  static constexpr char STMT[] = "BEGIN TRANSACTION;";
  ::sqlite::SQLite3Stmt Stmt = DB.prepare(STMT, sizeof(STMT) - 1);
  if (!Stmt) {
    return 1; /* Failed to prepare statement. */
  }
  if (Stmt.step() != SQLITE_DONE) {
    return 1; /* Failed to execute statement. */
  }
  return 0;
}

int SqliteDatabase::Commit() {
  static constexpr char STMT[] = "COMMIT;";
  ::sqlite::SQLite3Stmt Stmt = DB.prepare(STMT, sizeof(STMT) - 1);
  if (!Stmt) {
    return 1; /* Failed to prepare statement. */
  }
  if (Stmt.step() != SQLITE_DONE) {
    return 1; /* Failed to execute statement. */
  }
  return 0;
}

int SqliteDatabase::InsertIntoClass(std::string_view ClassName,
                                    std::string_view Name,
                                    std::string_view Type) {
  return 1; /* Not implemented yet. */
}

int SqliteDatabase::InsertIntoNamespace(std::string_view NamespaceName,
                                        std::string_view Child,
                                        std::string_view Type) {
  return 1; /* Not implemented yet. */
}

int SqliteDatabase::InsertSymbol(std::string_view Name, std::string File,
                                 int Line) {
  return 1; /* Not implemented yet. */
}

std::unique_ptr<TableIterator> SqliteDatabase::ClassBegin() {
  return nullptr; /* Not implemented yet. */
}
std::unique_ptr<TableIterator> SqliteDatabase::NamespaceBegin() {
  return nullptr; /* Not implemented yet. */
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
