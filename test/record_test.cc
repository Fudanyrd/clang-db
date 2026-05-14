#include <gtest/gtest.h>

/* Only after including gtest.h, else our TestHelper will not be defined. */
#include <clangdb.h>

namespace clang {

using TupleStrStrStr = std::tuple<std::string, std::string, std::string>;
#define arraysize(arr) (sizeof(arr) / sizeof((arr)[0]))

#define RunAction                                                              \
  database::InMemoryDatabase DB;                                               \
  std::unique_ptr<FrontendAction> action =                                     \
      std::make_unique<database::BuildDatabaseAction>(DB);                     \
  ASSERT_TRUE(Instance.ExecuteAction(*action))

#define CompareTuples(TupleVec, Expected)                                      \
  do {                                                                         \
    const size_t ExpectedSize = arraysize(Expected);                           \
    ASSERT_EQ(TupleVec.size(), ExpectedSize);                                  \
    for (size_t I = 0; I < ExpectedSize; I++) {                                \
      EXPECT_EQ(std::get<0>(TupleVec[I]), Expected[I][0]);                     \
      EXPECT_EQ(std::get<1>(TupleVec[I]), Expected[I][1]);                     \
      EXPECT_EQ(std::get<2>(TupleVec[I]), Expected[I][2]);                     \
    }                                                                          \
  } while (0)

#define CompareTuple(Tuple, Expected)                                          \
  do {                                                                         \
    EXPECT_EQ(std::get<0>(Tuple), Expected[0]);                                \
    EXPECT_EQ(std::get<1>(Tuple), Expected[1]);                                \
    EXPECT_EQ(std::get<2>(Tuple), Expected[2]);                                \
  } while (0)

TEST_F(TestHelper, CStruct) {
  PrepareParsingCXX("extern \"C\" struct point { float x, y; };\n");

  const char *Expected[][3] = {
      {"6extern5point", "1x", "6public1f"}, /* point::x */
      {"6extern5point", "1y", "6public1f"}, /* point::y */
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());

  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, CXXStruct) {
  PrepareParsingCXX(
      " struct point { mutable float x; protected: float y; };\n");

  const char *Expected[][3] = {
      /**
       * Note: the type of point::x can be "7mutable6public1f" or
       * "6public1f7mutable", depending on the {@link TypeofClassMember}
       * implementation.
       */
      {"5point", "1x", "7mutable6public1f"}, /* point::x */
      {"5point", "1y", "9protected1f"},      /* point::y */
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, CXXClass) {
  PrepareParsingCXX("class point { mutable float x; protected: float y; };\n");
  const char *Expected[][3] = {
      /**
       * Note: the type of point::x can be "7mutable7private1f" or
       * "7private1f7mutable", depending on the {@link TypeofClassMember}
       * implementation.
       */
      {"5point", "1x", "7mutable7private1f"}, /* point::x */
      {"5point", "1y", "9protected1f"},       /* point::y */
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, CXXUnion) {
  PrepareParsingCXX("union point { mutable float x; protected: float y; };\n");
  const char *Expected[][3] = {
      /**
       * Note: the type of point::x can be "7mutable6public1f" or
       * "6public1f7mutable", depending on the {@link TypeofClassMember}
       * implementation.
       */
      {"5point", "1x", "7mutable6public1f"}, /* point::x */
      {"5point", "1y", "9protected1f"},      /* point::y */
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, StaticMember) {
  PrepareParsingCXX(
      "struct foo { static char ch; static constexpr char A = \'A\'; };\n");
  const char *Expected[][3] = {
      {"3foo", "1A", "6static6public9constexpr1c"}, /* foo::A */
      {"3foo", "2ch", "6static6public1c"},          /* foo::ch */
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, NamespaceMember) {
  PrepareParsingCXX("namespace foo { char ch; constexpr char A = \'A\'; }\n"
                    "extern \"C\" int errno;\nconstexpr char TERM = 0;\n");
  const char *Expected[][3] = {
      {"", "3foo", "9namespace"},     /* namespace foo */
      {"", "4TERM", "9constexpr1c"},  /* global variable TERM */
      {"3foo", "1A", "9constexpr1c"}, /* foo::A */
      {"3foo", "2ch", "1c"},          /* foo::ch */
      {"6extern", "5errno", "1i"},    /* errno */
  };

  RunAction;
  auto &Actual = GetNamespaces(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, Expected);
}

TEST_F(TestHelper, BaseClasses) {
  PrepareParsingCXX(
      "class A{}; class B{}; class C : public virtual A, protected B {};\n"
      "struct D { struct Inner : public virtual A {}; };\n");
  const char *ExpectedNs[][3] = {
      {"", "1A", "5class"},                                      /* class A */
      {"", "1B", "5class"},                                      /* class B */
      {"", "1C", "5class7virtual6public7virtual1A9protected1B"}, /* class C */
      {"", "1D", "6struct"},                                     /* struct D */
  };
  const char *ExpectedCLS[3] = {
      "1D", "5Inner", "6struct6public7virtual6public7virtual1A", /* D::Inner */
  };

  RunAction;
  auto &Actual = GetNamespaces(DB);
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, ExpectedNs);

  Actual = GetClasses(DB);
  std::sort(Actual.begin(), Actual.end());
  /* There're too many constructor/destructors, so we only compare the relevant
   * one. */
  ASSERT_EQ(13U, Actual.size());
  CompareTuple(Actual[9], ExpectedCLS);
}

} /* namespace clang */
