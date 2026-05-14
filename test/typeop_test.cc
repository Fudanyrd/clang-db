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
      {"3foo", "Ci", "6public1v"}, /* foo::foo(int) */
      {"3foo", "Cv", "6public1v"}, /* foo::foo() */
      {"3foo", "Dv", "6public1v"}, /* foo::~foo() */
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

TEST_F(TestHelper, RAIITemplate) {
  PrepareParsingCXX("struct cint {\n"
                    "  template <typename NumTy> cint(NumTy a);\n"
                    "};");

  const char *Expected[3] = {
      "4cint",
      "C"                            /* constructor */
      "IN8template8typename5NumTyEE" /* template <typename NumTy> */
      "v"                            /* return type (provided by libclang) */
      "N8template8typename5NumTyE",  /* Parameter list. */
      "6public1v"};

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetClasses(DB);
  EXPECT_EQ(Actual.size(), 1U);

  EXPECT_EQ(std::get<0>(Actual[0]), Expected[0]);
  EXPECT_EQ(std::get<1>(Actual[0]), Expected[1]);
  EXPECT_EQ(std::get<2>(Actual[0]), Expected[2]);
}

TEST_F(TestHelper, CopyAndMove) {
  PrepareParsingCXX("struct foo {\n"
                    "  foo &operator=(foo&);\n"
                    "  foo &operator=(foo&&);\n"
                    "  foo(foo &); \n"
                    "};");

  const char *Expected[][3] = {
      {"3foo", "CRN3fooE", "6public1v"},        /* foo::foo(foo&) */
      {"3foo", "aSON3fooE", "6public7RN3fooE"}, /* foo::operator=(foo&) */
      {"3foo", "aSRN3fooE", "6public7RN3fooE"}, /* foo::operator=(foo&&) */
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

TEST_F(TestHelper, Arrays) {
  PrepareParsingCXX("extern \"C\" typedef int array_t [42];\n"
                    "extern \"C\" void fn(array_t, array_t*);\n"
                    "constexpr array_t foo = {1,2,3,4};\n"
                    "extern array_t bar[25];");

  const char *Expected[][3] = {
      {"", "3bar", "9A25_A42_i"}, /* array_t bar[42] */
      /**
       * constexpr, A42_i =>
       * `foo` is a constexpr array of size 42, type int.
       */
      {"", "3foo", "9constexpr5A42_i"}, /* array_t foo */
      /**
       * Array parameter is passed as pointer.
       */
      {"6extern", "2fnPiPA42_i", "v"}, /* void fn(array_t) */
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

TEST_F(TestHelper, VirtualMethod) {
  PrepareParsingCXX("struct A { virtual int fn() = 0; };\n"
                    "struct B : public A { int fn() override; };");

  const char *Expected[][3] = {
      "1A", "2fnv", "7virtual6public1i", /* A::fn() */
      "1B", "2fnv", "6public1i",         /* B::fn() */
  };

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(Instance.ExecuteAction(*action));

  std::vector<TupleStrStrStr> &Actual = GetClasses(DB);
  ASSERT_EQ(Actual.size(), 8);
  std::sort(Actual.begin(), Actual.end());
  TupleStrStrStr *Methods[] = {&Actual[0], &Actual[4]};

  for (size_t I = 0; I < arraysize(Expected); I++) {
    TupleStrStrStr &Method = *Methods[I];
    EXPECT_EQ(std::get<0>(Method), Expected[I][0]);
    EXPECT_EQ(std::get<1>(Method), Expected[I][1]);
    EXPECT_EQ(std::get<2>(Method), Expected[I][2]);
  }
}

} /* namespace clang */
