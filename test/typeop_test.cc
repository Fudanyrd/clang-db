/**
 * Test C/C++'s operation of types.
 *
 * Existing tests:
 * <ul>
 *   <li>using</li>
 *   <li>typedef</li>
 *   <li>Construtor/destructor of C++ classes</li>
 * </ul>
 */
#include <gtest/gtest.h>

/* Only after including gtest.h, else our TestHelper will not be defined. */
#include <clangdb.h>

namespace clang {

using TupleStrStrStr = std::tuple<std::string, std::string, std::string>;

#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))

TEST_F(TestHelper, BuiltinTy) {
  PrepareParsingCXX("struct point {\n"
                    "  typedef float  _XTy;\n"
                    "  using _YTy = int;\n"
                    "  \n"
                    "  _XTy &x();\n"
                    "  _YTy &y();\n"
                    "};");

  const char *Expected[][3] = {
      {"5point", "1xv", "6public2Rf"},
      {"5point", "1yv", "6public2Ri"},
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetClasses(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));
  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

TEST_F(TestHelper, TemplateTy) {
  PrepareParsingCXX("template <typename _Key, typename _Value>\n"
                    "struct Pair {\n"
                    "  using key_type = _Key;\n"
                    "  typedef _Value value_type;\n"
                    "  key_type &key();\n"
                    "  _Value &value();\n"
                    "};");

  const char *Expected[][3] = {
      {"4PairIN8template8typename4_KeyEN8template8typename6_ValueEE", "3keyv",
       "6public26RN8template8typename4_KeyE"},
      {"4PairIN8template8typename4_KeyEN8template8typename6_ValueEE", "5valuev",
       "6public28RN8template8typename6_ValueE"},
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetClasses(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));
  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

TEST_F(TestHelper, Scope) {
  PrepareParsingCXX("namespace foo {\n"
                    " using T = int;\n"
                    " T fn1();\n"
                    " namespace bar {\n"
                    "   using T = float;\n"
                    "   T fn();\n"
                    " }\n"
                    " T fn2();\n"
                    "}");

  const char *Expected[][3] = {
      {"", "3foo", "9namespace"}, {"3foo", "3bar", "9namespace"},
      {"3foo", "3fn1v", "i"},     {"3foo", "3fn2v", "i"},
      {"3foo3bar", "2fnv", "f"},
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));
  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

TEST_F(TestHelper, Linkage) {
  PrepareParsingCXX("using T = float;\n"
                    "extern \"C\" T fn();\n"
                    "namespace foo {\n"
                    "  using T = int;\n"
                    "  T fn();\n"
                    "}");

  const char *Expected[][3] = {
      {"", "3foo", "9namespace"},
      {"3foo", "2fnv", "i"},
      {"6extern", "2fnv", "f"},
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));
  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

TEST_F(TestHelper, RAII) {
  PrepareParsingCXX("struct foo {\n"
                    "  foo();\n"
                    "  foo(int);\n"
                    "  ~foo();\n"
                    "};");

  /**
   * Whole mangled name for `foo::foo()' is `_ZN3fooCEv',
   * the same rule applies to destructor `D'.
   *
   * (const type is marked with `K').
   */
  const char *Expected[][3] = {
      {"3foo", "3fooCi", "6public1v"}, /* foo::foo(int) */
      {"3foo", "3fooCv", "6public1v"}, /* foo::foo() */
      {"3foo", "3fooDv", "6public1v"}, /* foo::~foo() */
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetClasses(DB);
  EXPECT_EQ(Actual.size(), arraysize(Expected));
  std::sort(Actual.begin(), Actual.end());

  for (size_t I = 0; I < arraysize(Expected); I++) {
    EXPECT_EQ(std::get<0>(Actual[I]), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Actual[I]), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Actual[I]), Expected[I][2]);
  }
}

} /* namespace clang */
