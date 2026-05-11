#include <algorithm>
#include <gtest/gtest.h>

#include <clangdb.h>

namespace clang {
TEST_F(TestHelper, NamespaceNesting) {
  static const char code[] =
      "namespace ns1 { namespace ns2 { namespace ns3 {}}}\n"
      "namespace ns4 {}\n";
  const char *ExpectedSortedResult[][2] = {
      {"", "3ns1"},
      {"", "3ns4"},
      {"3ns1", "3ns2"},
      {"3ns13ns2", "3ns3"},
  };
  size_t Length =
      sizeof(ExpectedSortedResult) / sizeof(ExpectedSortedResult[0]);

  PrepareParsingCXX(code);
  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);

  ASSERT_TRUE(Instance.ExecuteAction(*action));
  std::vector<std::tuple<std::string, std::string, std::string>> Namespaces;
  Namespaces = DB.GetNamespaces();
  std::sort(Namespaces.begin(), Namespaces.end());

  for (auto i = 0U; i < Length; i++) {
    EXPECT_EQ(std::get<0>(Namespaces[i]), ExpectedSortedResult[i][0]);
    EXPECT_EQ(std::get<1>(Namespaces[i]), ExpectedSortedResult[i][1]);
  }
}

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

TEST_F(TestHelper, ClassNesting) {
  static const char code[] = "struct Foo { class Bar {}; };\n"
                             "class Baz {};\n";
  const char *TableClass[][2] = {
      {"3Foo", "3Bar"},
  };
  const char *TableNamespace[][2] = {
      {"", "3Baz"},
      {"", "3Foo"},
  };

  PrepareParsingCXX(code);
  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<std::tuple<std::string, std::string, std::string>> TableRows;
  TableRows = DB.GetClasses();
  std::sort(TableRows.begin(), TableRows.end());
  for (auto i = 0U; i < arraysize(TableClass); i++) {
    EXPECT_EQ(std::get<0>(TableRows[i]), TableClass[i][0]);
    EXPECT_EQ(std::get<1>(TableRows[i]), TableClass[i][1]);
  }

  TableRows = DB.GetNamespaces();
  std::sort(TableRows.begin(), TableRows.end());
  for (auto i = 0U; i < arraysize(TableNamespace); i++) {
    EXPECT_EQ(std::get<0>(TableRows[i]), TableNamespace[i][0]);
    EXPECT_EQ(std::get<1>(TableRows[i]), TableNamespace[i][1]);
  }
}

} // namespace clang
