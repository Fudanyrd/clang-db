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

TEST_F(TestHelper, FunctionPointer) {
  PrepareParsingCXX("int fn(int (*g)(void));");
  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  const char *ExpectedTuple[3] = {
      "",
      "2fnPFivE",
      "i" /* return type */,
  };
  std::vector<TupleStrStrStr> &Actual = GetNamespaces(DB);
  EXPECT_EQ(Actual.size(), 1U);

  {
    EXPECT_EQ(std::get<0>(Actual[0]), ExpectedTuple[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), ExpectedTuple[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), ExpectedTuple[2]);
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

TEST_F(TestHelper, OverloadedOperator) {
  PrepareParsingCXX("struct Int {\n"
                    "  Int &operator+=(Int &Other);\n"
                    "};\n"
                    "Int operator+(Int &LHS, Int &RHS);");

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  const char *ClassRecord[3] = {"3Int", "pLRN3IntE", "6public7RN3IntE"};
  const char *GlobalRecord[3] = {"", "plRN3IntERN3IntE", "N3IntE"};
  {
    auto &Actual = GetClasses(DB);
    ASSERT_EQ(Actual.size(), 1U);
    EXPECT_EQ(std::get<0>(Actual[0]), ClassRecord[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), ClassRecord[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), ClassRecord[2]);
  }
  {
    auto &Actual = GetNamespaces(DB);
    ASSERT_TRUE(Actual.size() == 2U);
    EXPECT_EQ(std::get<0>(Actual[1]), GlobalRecord[0]);
    EXPECT_EQ(std::get<1>(Actual[1]), GlobalRecord[1]);
    EXPECT_EQ(std::get<2>(Actual[1]), GlobalRecord[2]);
  }
}

TEST_F(TestHelper, TemplateOverloadedOperator) {
  PrepareParsingCXX("struct Int {\n"
                    "  Int &operator+=(Int &Other);\n"
                    "};\n"
                    "template <typename T>\n"
                    "Int operator+(T num, Int &RHS);");

  const char *GlobalRecord[3] = {
      "",
      "pl"                       /* operator + */
      "IN8template8typename1TEE" /* template <typename T> */
      "N3IntE"                   /* returns Int */
      "N8template8typename1TE"   /* first parameter T */
      "RN3IntE",                 /* second parameter Int& */
      "N3IntE"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));
  {
    auto &Actual = GetNamespaces(DB);
    ASSERT_TRUE(Actual.size() == 2U);
    EXPECT_EQ(std::get<0>(Actual[1]), GlobalRecord[0]);
    EXPECT_EQ(std::get<1>(Actual[1]), GlobalRecord[1]);
    EXPECT_EQ(std::get<2>(Actual[1]), GlobalRecord[2]);
  }
}

TEST_F(TestHelper, ConversionDecl) {
  PrepareParsingCXX("struct Int {\n"
                    "  operator int() const;\n"
                    "};\n");

  const char *GlobalRecord[3] = {
      "3Int", "cv" /* conversion operator */ "i" /* int */ "v" /* parameters */,
      "5const6public1i"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));
  {
    auto &Actual = GetClasses(DB);
    ASSERT_TRUE(Actual.size() == 1U);
    EXPECT_EQ(std::get<0>(Actual[0]), GlobalRecord[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), GlobalRecord[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), GlobalRecord[2]);
  }
}

TEST_F(TestHelper, TemplateConversionDecl) {
  PrepareParsingCXX("struct Int {\n"
                    "  template <typename T> operator T() const;\n"
                    "};\n");

  const char *ClassRecord[3] = {
      "3Int",
      "cv"                       /* convert operator */
      "N8template8typename1TE"   /* convert to T */
      "IN8template8typename1TEE" /* template <typename T> */
      "N8template8typename1TE"   /* return type */
      "v",                       /* parameter list */
      "5const"
      "6public"
      "22N8template8typename1TE"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));
  {
    auto &Actual = GetClasses(DB);
    ASSERT_TRUE(Actual.size() == 1U);
    EXPECT_EQ(std::get<0>(Actual[0]), ClassRecord[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), ClassRecord[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), ClassRecord[2]);
  }
}

TEST_F(TestHelper, DependentType) {
  PrepareParsingCXX("template <typename T>\n"
                    "typename T::iterator GetBeg(T&a) { return a.begin(); }\n");

  const char *GlobalDecl[3] = {
      "",
      "6GetBeg"
      "IN8template8typename1TEE"        /* template <typename T> */
      "N8template8typename1T8iteratorE" /* T::iterator */
      "RN8template8typename1TE",        /* T& */
      "N8template8typename1T8iteratorE" /* return T::iterator */
  };
  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));
  {
    auto &Actual = GetNamespaces(DB);
    ASSERT_TRUE(Actual.size() == 1U);
    EXPECT_EQ(std::get<0>(Actual[0]), GlobalDecl[0]);
    EXPECT_EQ(std::get<1>(Actual[0]), GlobalDecl[1]);
    EXPECT_EQ(std::get<2>(Actual[0]), GlobalDecl[2]);
  }
}

} /* namespace clang */
