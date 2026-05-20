/**
 * Loads a database file and interactively reads
 * a symbol from user, and prints the location of it.
 *
 * Query format: tokens joined by '::', e.g. std::vector::push_back.
 * Starting from Commit 47cbd009, vague query is supported (but slower),
 * e.g. `vector::size_type` will find you the  location of
 * `std::vector::size_type`.
 *
 * How does it work: gradually descend into nested scopes.
 * e.g. for the query above, '' (translation unit) -> std (namespace) ->
 * vector (class) -> push_back (method)
 *
 * <h3>Known Issues</h3>
 * The greatest would be lack of support for anonymous namespaces,
 * e.g.
 * <blockquote><pre>
 *  namespace foo { namespace { int x; } }
 * </pre></blockquote>
 * is stored internally as '3foo01x', therefore querying 'foo::x' will not work.
 */

#include <clangdb.h>
#include <iostream>

namespace clang {

/**
 * For quickly push and pop tokens.
 */
struct StringStack {
  /**
   * Each time, the top of Offsets equals to length
   * of Buffer [invariant].
   */
  std::string Buffer;
  std::vector<size_t> Offsets;

  StringStack() {
    /* Push top-level unit. */
    Offsets.push_back(0);
  }
  ~StringStack() = default;

  void push(std::string_view Str) {
    Buffer += Str;
    Offsets.push_back(Buffer.size());
  }

  void reset() {
    Buffer.clear();
    Offsets.resize(1);
    Offsets[0] = 0;
  }

  void pop() {
    if (Offsets.size() <= 1) {
      Buffer.clear();
      Offsets.clear();
      return;
    }
    Offsets.pop_back();
    size_t RemLength = Offsets.back();
    Buffer.resize(RemLength);
  }
};

namespace database _CLANGDB_VISIBILITY {

class Retriever {
private:
  SqliteDatabase &DB;
  std::ostream &Output;
  mutable int NumResults;

  void Descend(StringStack &Stack, const std::string &Query, size_t StartIdx);

  static void failure(int errcode, int line) {
    std::cerr << "At line " << line << ": ";
    std::cerr << "Database error: " << errcode << std::endl;
    /* abort(); */
  }

  void Yield(StringStack &Stack);

public:
  Retriever(SqliteDatabase &Data, std::ostream &Out = std::cout)
      : DB(Data), Output(Out) {}

  void Query(const std::string &Key);
};

void Retriever::Yield(StringStack &Stack) {
  /**
   * Lookup in the Table symbol,
   * Firstly use exact matching; if not found,
   * use prefix matching.
   */
  static const char ExactStmt[] =
      "SELECT file, line FROM " ClangDBTableSymbol " WHERE name = ?;";

  ::sqlite::SQLite3Stmt Stmt = DB.GetDB().prepare(ExactStmt);
  int Result = ::sqlite::Binder<1, std::string_view>::bind(Stmt, Stack.Buffer);
  if (Result != SQLITE_OK) {
    failure(DB.GetDB().errcode(), __LINE__);
    return;
  }
  while (Stmt.step() == SQLITE_ROW) {
    std::string File;
    int Line;
    ::sqlite::Row<0, std::string, int> Row(File, Line);
    Result = Row.get_one(Stmt);
    if (Result != SQLITE_OK) {
      failure(DB.GetDB().errcode(), __LINE__);
      break;
    }
    Output << File << ":" << Line << std::endl;
    NumResults++;
  }

  if (NumResults) {
    return;
  }

  Stack.push("%"); /* used in a SQL LIKE Stmt */
  static const char PrefixStmt[] =
      "SELECT file, line FROM " ClangDBTableSymbol " WHERE name LIKE ?;";
  Stmt = DB.GetDB().prepare(PrefixStmt);
  Result = ::sqlite::Binder<1, std::string_view>::bind(Stmt, Stack.Buffer);
  if (Result != SQLITE_OK) {
    failure(DB.GetDB().errcode(), __LINE__);
    return;
  }
  while (Stmt.step() == SQLITE_ROW) {
    std::string File;
    int Line;
    ::sqlite::Row<0, std::string, int> Row(File, Line);
    Result = Row.get_one(Stmt);
    if (Result != SQLITE_OK) {
      failure(DB.GetDB().errcode(), __LINE__);
      break;
    }
    Output << File << ":" << Line << std::endl;
    NumResults++;
  }
  Stack.pop(); /* Pop '%' pushed previously */
}

void Retriever::Query(const std::string &Key) {
  /* Use precise searching. */
  NumResults = 0;
  StringStack Stack;
  Descend(Stack, Key, 0);

  if (NumResults > 0) {
    return;
  }

  /**
   * Resort to non-precise matching.
   *
   * It works like this: retrieve the first token
   * in Key, and try to use prefix-match on the
   * member column, to get its parent declaration
   * in the AST. Then, `Descend` can be used.
   *
   * This involves scanning the entire table, because
   * the index does not work.
   */
  const char *Data = Key.data();
  const char *FirstTokenEnd = strchr(Data, ':');
  size_t FirstTokenLen =
      (FirstTokenEnd == nullptr) ? Key.size() : (size_t)(FirstTokenEnd - Data);
  std::string FirstToken = std::to_string(FirstTokenLen);
  FirstToken += std::string(Data, FirstTokenLen);
  FirstToken.push_back('%'); /* used in a SQL LIKE Stmt */

  static const char *const STMTS[] = {
      "SELECT DISTINCT(name) FROM " ClangDBTableNamespace
      " WHERE member LIKE ?;",
      "SELECT DISTINCT(name) FROM " ClangDBTableClass " WHERE member LIKE ?;",
  };
  Stack.reset();
  for (const char *STMT : STMTS) {
    /* Search member which is like FirstToken in namespace. */
    ::sqlite::SQLite3Stmt Stmt = DB.GetDB().prepare(STMT);
    int Result = ::sqlite::Binder<1, std::string_view>::bind(Stmt, FirstToken);
    if (Result != SQLITE_OK) {
      failure(DB.GetDB().errcode(), __LINE__);
      return;
    }
    while (Stmt.step() == SQLITE_ROW) {
      std::string Name; /* the DeclContext holding current decl */
      ::sqlite::Row<0, std::string> Row(Name);
      Result = Row.get_one(Stmt);
      if (Result != SQLITE_OK) {
        failure(DB.GetDB().errcode(), __LINE__);
        break;
      }
      /* Found a valid prefix, can use Descend again. */
      Stack.push(Name);
      Descend(Stack, Key, 0);
      Stack.pop();
    }
  }

  if (NumResults == 0) {
    Output << "No result found for query: " << Key << std::endl;
  }
}

void Retriever::Descend(StringStack &Stack, const std::string &Query,
                        size_t StartIdx) {
  size_t Length = Query.size();
  assert(Query[StartIdx] != ':');
  size_t End = StartIdx + 1;
  while (End < Length && Query[End] != ':') {
    ++End;
  }
  std::string Token = std::to_string(End - StartIdx);
  Token += Query.substr(StartIdx, End - StartIdx);
  Token.push_back('%'); /* used in a SQL LIKE Stmt */

  /**
   * A little note about why use prefix matching (LIKE "SomeName%")
   * They will not disable database indexing; and correctly
   * handles template class (e.g. 6vectorI...E) and function
   * (e.g. 2fnii, with argument list.)
   */
  static const char *const STMTs[] = {
      "SELECT DISTINCT(member), type FROM " ClangDBTableNamespace
      " WHERE (name = ?) AND (member LIKE ?);",
      "SELECT DISTINCT(member), type FROM " ClangDBTableClass
      " WHERE (name = ?) AND (member LIKE ?);",
  };
  /* Look up from the Table namespace. */
  for (const char *STMT : STMTs) {
    std::vector<bool> IsNamespaces;
    std::vector<std::string> Members;
    Members.reserve(4);
    /* Stmt Initialize */ {
      ::sqlite::SQLite3Stmt Stmt = DB.GetDB().prepare(STMT);
      std::string Member;
      std::string Type;
      int Result =
          ::sqlite::Binder<1, const std::string &, const std::string &>::bind(
              Stmt, Stack.Buffer, Token);
      if (Result != SQLITE_OK) {
        failure(DB.GetDB().errcode(), __LINE__);
        return;
      }

      while (Stmt.step() == SQLITE_ROW) {
        ::sqlite::Row<0, std::string, std::string> Row(Member, Type);
        Result = Row.get_one(Stmt);
        if (Result != SQLITE_OK) {
          failure(DB.GetDB().errcode(), __LINE__);
          break;
        }
        IsNamespaces.push_back(Type == "9namespace");
        Members.push_back(Member);
      }

    } /* Stmt finalize */

    size_t NumMembers = Members.size();
    for (size_t i = 0; i < NumMembers; ++i) {
      const std::string &Member = Members[i];
      bool IsNamespace = IsNamespaces[i];
      Stack.push(Member);
      if (End >= Length) {
        if (IsNamespace) {
          /* namespace is not stored in the symbol table. */
          Output << "(namespace)" << std::endl;
          NumResults += 1;
        } else {
          Yield(Stack);
        }
      } else {
        Descend(Stack, Query, End + 2);
      }
      Stack.pop();
    }
  }
}

} /* namespace database _CLANGDB_VISIBILITY */
} /* namespace clang */

/**
 * Synopsis
 * retriever [SQLite Database Path]
 */
int main(int argc, char **argv) {
  const char *IFile = argv[1];
  if (IFile == nullptr) {
    std::cerr << "Usage: retriever [SQLite Database Path]" << std::endl;
    return 1;
  }

  clang::database::SqliteDatabase DB(IFile, SQLITE_OPEN_READONLY);
  if (!DB) {
    std::cerr << "Failed to open database file: " << IFile << std::endl;
    return 1;
  }

  clang::database::Retriever R(DB);
  std::string Query;
  while (std::cin >> Query) {
    if (Query.empty()) {
      continue;
    }
    R.Query(Query);
  }
  return 0;
}
