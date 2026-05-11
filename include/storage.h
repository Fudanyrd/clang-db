#ifndef __STORAGE_H__
#define __STORAGE_H__ (1)

#include <llvm/Support/raw_ostream.h>
#include <string_view>
#include <tuple>
#include <vector>

#include "common.h"

namespace clang {
namespace database _CLANGDB_VISIBILITY {

class TableIterator {
public:
  virtual ~TableIterator() = default;
  virtual int
  Next(std::tuple<std::string, std::string, std::string> &OutputTuple) = 0;
};

class DatabaseInterface {
public:
  virtual ~DatabaseInterface() = default;

  virtual int TransactionBegin() = 0;
  virtual int Commit() = 0;

  virtual std::unique_ptr<TableIterator> ClassBegin() = 0;
  virtual std::unique_ptr<TableIterator> NamespaceBegin() = 0;

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
};

/**
 * Keep two tables in memory. Only used for testing purposes.
 */
class InMemoryDatabase : public DatabaseInterface {
private:
  std::vector<std::tuple<std::string, std::string, std::string>> Classes;
  std::vector<std::tuple<std::string, std::string, std::string>> Namespaces;
  bool InTransaction = false;

public:
  struct Iterator : public TableIterator {
    std::vector<std::tuple<std::string, std::string, std::string>> &Table;
    size_t Offset;

    Iterator(
        std::vector<std::tuple<std::string, std::string, std::string>> &Table)
        : Table(Table), Offset(0) {}

    int Next(std::tuple<std::string, std::string, std::string> &OutputTuple)
        override {
      if (Offset >= Table.size()) {
        return 1; /* No more records */
      }
      OutputTuple = Table[Offset];
      Offset++;
      return 0;
    }
  };

  InMemoryDatabase() = default;
  ~InMemoryDatabase() override = default;

  std::unique_ptr<TableIterator> ClassBegin() override {
    return std::make_unique<Iterator>(Classes);
  }

  std::unique_ptr<TableIterator> NamespaceBegin() override {
    return std::make_unique<Iterator>(Namespaces);
  }

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
};

} // namespace database _CLANGDB_VISIBILITY
} /* namespace clang */

#endif /* __STORAGE_H__ */
