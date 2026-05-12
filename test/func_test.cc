#include <gtest/gtest.h>

/* Only after including gtest.h, else our TestHelper will not be defined. */
#include <clangdb.h>

namespace clang {

using TupleStrStrStr = std::tuple<std::string, std::string, std::string>;

#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))

TEST_F(TestHelper, GlobalFunction) {
  PrepareParsingCXX("namespace foo {\n"
                    "  template <typename T> T self(T &);\n"
                    "}\n");

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  /**
   * Table namespace.
   */
  const char *Expected[][3] = {
      {"", "3foo", "9namespace"},
      {"3foo",
       "4selfIN8template8typename1TEEN8template8typename1TERN8template8typename"
       "1TE",
       "N8template8typename1TE"},
  };

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));

  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

TEST_F(TestHelper, TemplatePackExpand) {
  PrepareParsingCXX("template <typename ...Args>\n"
                    "void fn(Args& ...args, Args* ...argsPtr, int);");

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  const char *ExpectedTuple[3] = {
      "",
      "2fnIN8template8typename4Args8decltype1zEEvRN8template8typename4Args8decl"
      "type1zEPN8template8typename4Args8decltype1zEi",
      "v" /* return type */
  };

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), 1U);

  {
    EXPECT_EQ(std::get<0>(Actual[0]), ExpectedTuple[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), ExpectedTuple[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), ExpectedTuple[2]);
  }
}

} /* namespace clang */
