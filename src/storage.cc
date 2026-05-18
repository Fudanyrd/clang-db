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
  static constexpr char STMT[] =
      "INSERT INTO " ClangDBTableClass " VALUES (?, ?, ?);";
  ::sqlite::SQLite3Stmt SqliteStmt = DB.prepare(STMT, sizeof(STMT) - 1);
  if (!SqliteStmt) {
    return 1; /* Failed to prepare statement. */
  }

  if (::sqlite::Binder<1, std::string_view, std::string_view,
                       std::string_view>::bind(SqliteStmt, ClassName, Name,
                                               Type)) {
    llvm::errs() << this->DB.err_message();
    return 1; /* Failed to bind parameters. */
  }
  if (SqliteStmt.step() != SQLITE_DONE) {
    return 1; /* Failed to execute statement. */
  }
  return 0;
}

int SqliteDatabase::InsertIntoNamespace(std::string_view NamespaceName,
                                        std::string_view Child,
                                        std::string_view Type) {
  static constexpr char STMT[] =
      "INSERT INTO " ClangDBTableNamespace " VALUES (?, ?, ?);";
  ::sqlite::SQLite3Stmt SqliteStmt = DB.prepare(STMT, sizeof(STMT) - 1);
  if (!SqliteStmt) {
    return 1; /* Failed to prepare statement. */
  }
  if (::sqlite::Binder<1, std::string_view, std::string_view,
                       std::string_view>::bind(SqliteStmt, NamespaceName, Child,
                                               Type)) {
    return 1; /* Failed to bind parameters. */
  }
  if (SqliteStmt.step() != SQLITE_DONE) {
    return 1; /* Failed to execute statement. */
  }
  return 0; /* Not implemented yet. */
}

int SqliteDatabase::InsertSymbol(std::string_view Name, std::string File,
                                 int Line) {
  static constexpr char STMT[] =
      "INSERT INTO " ClangDBTableSymbol " VALUES (?, ?, ?);";
  ::sqlite::SQLite3Stmt SqliteStmt = DB.prepare(STMT, sizeof(STMT) - 1);
  if (!SqliteStmt) {
    return 1; /* Failed to prepare statement. */
  }
  if (::sqlite::Binder<1, std::string_view, std::string_view, int>::bind(
          SqliteStmt, Name, File, Line)) {
    return 1; /* Failed to bind parameters. */
  }

  if (SqliteStmt.step() != SQLITE_DONE) {
    return 1; /* Failed to execute statement. */
  }
  return 0; /* Not implemented yet. */
}

void SqliteDatabase::CreateTableAndIndex() {
  if (!DB) {
    clangdb_check_internal(false && "Failed to open database.");
  }

#define ExecuteSQL(sqlStmt)                                                    \
  do {                                                                         \
    ::sqlite::SQLite3Stmt Stmt = DB.prepare(sqlStmt, sizeof(sqlStmt) - 1);     \
    if (!Stmt) {                                                               \
      clangdb_check_internal(false &&                                          \
                             "Failed to prepare statement: " sqlStmt);         \
    }                                                                          \
    if (Stmt.step() != SQLITE_DONE) {                                          \
      clangdb_check_internal(false &&                                          \
                             "Failed to execute statement: " sqlStmt);         \
    }                                                                          \
  } while (0)

  ExecuteSQL("CREATE TABLE IF NOT EXISTS " ClangDBTableClass " ("
             "  name TEXT NOT NULL,"
             "  member TEXT NOT NULL,"
             "  type TEXT NOT NULL"
             ");");

  ExecuteSQL("CREATE TABLE IF NOT EXISTS " ClangDBTableNamespace " ("
             "  name TEXT NOT NULL,"
             "  member TEXT NOT NULL,"
             "  type TEXT NOT NULL"
             ");");

  ExecuteSQL("CREATE TABLE IF NOT EXISTS " ClangDBTableSymbol " ("
             "  name TEXT NOT NULL,"
             "  file TEXT NOT NULL,"
             "  line INTEGER NOT NULL"
             ");");

  /* Create indices */
  ExecuteSQL("CREATE INDEX IF NOT EXISTS idx_class_name ON " ClangDBTableClass
             " (name, member);");

  ExecuteSQL(
      "CREATE INDEX IF NOT EXISTS idx_namespace_name ON " ClangDBTableNamespace
      "(name, member);");

  ExecuteSQL("CREATE INDEX IF NOT EXISTS idx_symbol_name ON " ClangDBTableSymbol
             " (name);");
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */
