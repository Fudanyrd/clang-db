#include "test.h"

namespace clang {

TEST_F(TestHelper, CStruct) {
  PrepareParsingCXX("extern \"C\" struct point { float x, y; };\n");

  const char *Expected[][3] = {
      {"5point", "1x", "6public1f"}, /* point::x */
      {"5point", "1y", "6public1f"}, /* point::y */
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
      {"3foo", "1A", "6static6public9constexpr2Kc"}, /* foo::A */
      {"3foo", "2ch", "6static6public1c"},           /* foo::ch */
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
      {"", "3foo", "9namespace"},      /* namespace foo */
      {"", "4TERM", "9constexpr2Kc"},  /* global variable TERM */
      {"", "5errno", "1i"},            /* errno */
      {"3foo", "1A", "9constexpr2Kc"}, /* foo::A */
      {"3foo", "2ch", "1c"},           /* foo::ch */
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
      {"", "1A", "5class"}, /* class A */
      {"", "1B", "5class"}, /* class B */
      {"", "1C",
       "5class7virtual"       /* class C */
       "6public7virtual4N1AE" /* public virtual A */
       "9protected4N1BE"},    /* protected B */
      {"", "1D", "6struct"},  /* struct D */
  };
  const char *ExpectedCLS[3] = {
      "1D", "5Inner",
      "6struct6public7virtual6public7virtual4N1AE", /* D::Inner */
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

TEST_F(TestHelper, Enum) {
  PrepareParsingCXX("enum Color { RED, GREEN };\n");
  const char *ExpectedCLS[][3] = {
      {"5Color", "3RED", "6public4enum1i"},
      {"5Color", "5GREEN", "6public4enum1i"},
  };

  RunAction;
  auto &Actual = GetClasses(DB);
  ASSERT_EQ(2U, Actual.size());
  std::sort(Actual.begin(), Actual.end());
  CompareTuples(Actual, ExpectedCLS);
}

} /* namespace clang */
