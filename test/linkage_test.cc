#include <gtest/gtest.h>

/* Only after including gtest.h, else our TestHelper will not be defined. */
#include <clangdb.h>

namespace clang {

using TupleStrStrStr = std::tuple<std::string, std::string, std::string>;
#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))

TEST_F(TestHelper, CLinkageSingle) {
  PrepareParsingCXX("extern \"C\" int fn(void);\n");
  const char *Expected[3] = {"6extern", "2fnv", "i"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), 1U);

  EXPECT_EQ(std::get<0>(Actual[0]), Expected[0]);
  EXPECT_EQ(std::get<1>(Actual[0]), Expected[1]);
  EXPECT_EQ(std::get<2>(Actual[0]), Expected[2]);
}

TEST_F(TestHelper, CLinkageBlock) {
  PrepareParsingCXX("extern \"C\" { int fn(void); }\n");
  const char *Expected[3] = {"6extern", "2fnv", "i"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), 1U);

  EXPECT_EQ(std::get<0>(Actual[0]), Expected[0]);
  EXPECT_EQ(std::get<1>(Actual[0]), Expected[1]);
  EXPECT_EQ(std::get<2>(Actual[0]), Expected[2]);
}

TEST_F(TestHelper, CXXLinkage) {
  PrepareParsingCXX("extern \"C++\" {"
                    " namespace foo { int fn(void); } "
                    "void fn(int); }");
  const char *Expected[][3] = {
      {"", "2fni", "v"}, {"", "3foo", "9namespace"}, {"3foo", "2fnv", "i"}};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  std::sort(Actual.begin(), Actual.end());

  const size_t ExpectedSize = arraysize(Expected);
  EXPECT_EQ(Actual.size(), ExpectedSize);
  for (size_t I = 0; I < ExpectedSize; I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

/**
 * Commit 33ae90cf has a bug -- `MangleRecordDecl` does not
 * handle `LinkageSpecDecl` => assertion failure.
 */
TEST_F(TestHelper, RecordSearch) {
  PrepareParsingCXX("extern \"C\" { struct point { float x, y; }; }\n"
                    "point fn(point *);\n");

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  const char *Expected[][3] = {
      {"", "2fnPN6extern5pointE", "N6extern5pointE"}, /* fn */
      {"6extern", "5point", "6struct"},               /* struct point */
  };
  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  std::sort(Actual.begin(), Actual.end());

  const size_t ExpectedSize = arraysize(Expected);
  EXPECT_EQ(Actual.size(), ExpectedSize);
  for (size_t I = 0; I < ExpectedSize; I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

} /* namespace clang */
