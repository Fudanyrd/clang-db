/*
 * Starting from Commit 3e29860a,
 * the `DatabaseInterface` manages one more table to
 * map a <b>symbol</b> to its <b>source location</b>.
 *
 * This testsuite checks these parts of code.
 */

#include "test.h"

namespace clang {

using TupleStrLoc = std::tuple<std::string, std::string, int>;

/**
 * @param Tuple  (TupleStrLoc) The tuple to compare.
 * @param Expected (string) The expected symbol name.
 */
#define CompareSymbol(Tuple, Expected)                                         \
  do {                                                                         \
    EXPECT_EQ(std::get<0>(Tuple), Expected);                                   \
  } while (0)

#define CompareSymbols(TupleVec, ExpectedSymbols)                              \
  do {                                                                         \
    const size_t ExpectedSize = arraysize(ExpectedSymbols);                    \
    ASSERT_EQ(TupleVec.size(), ExpectedSize);                                  \
    for (size_t I = 0; I < ExpectedSize; I++) {                                \
      EXPECT_EQ(std::get<0>(TupleVec[I]), ExpectedSymbols[I]);                 \
    }                                                                          \
  } while (0)

/**
 * It covers function and CXXRecord.
 */
TEST_F(TestHelper, SymbolTable) {
  const char *Symbols[] = {
      "1A",        "1A2fnv",              /* A::fn() */
      "1ADv",                             /* A::~A() */
      "1AaSON1AE",                        /* A::operator=(A&&) */
      "1AaSRN1AE",                        /* A::operator=(A&) */
      "1B",        "1B2fnv",   "1BDv",    /* B::~B() */
      "1BaSON1BE",                        /* B::operator=(B&&) */
      "1BaSRN1BE",                        /* B::operator=(B&) */
      "3foo",      "3foo3bar", "4myfniz", /* myfn(int, ...) */
  };
  PrepareParsingCXX("struct foo { struct bar { }; };\n"
                    "struct A { virtual int fn() = 0; }; \n"
                    "struct B: public A { int fn(); };\n"
                    "int myfn(int, ...);\n");

  RunAction;
  auto &Actual = GetSymbols(DB);
  std::sort(Actual.begin(), Actual.end(),
            [](const TupleStrLoc &A, const TupleStrLoc &B) {
              return std::get<0>(A) < std::get<0>(B);
            });
  CompareSymbols(Actual, Symbols);
}

/**
 * It covers template declarations.
 */
TEST_F(TestHelper, SymbolTable2) {
  const char *Expected[] = {
      "3fooIN8template8typename1TEE",     /* foo<T> */
      "3fooIN8template8typename1TEE3obj", /* foo<T>::obj */
      "4fromIN8template8typename1TEEN8template8typename1TERN8template8typename1"
      "TE",
      /* T from<T>(T&) */
  };
  PrepareParsingCXX("template <typename T> T from(T&);\n"
                    "template <typename T> struct foo { T obj; };\n");

  RunAction;
  auto &Actual = GetSymbols(DB);
  std::sort(Actual.begin(), Actual.end(),
            [](const TupleStrLoc &A, const TupleStrLoc &B) {
              return std::get<0>(A) < std::get<0>(B);
            });
  CompareSymbols(Actual, Expected);
}

/**
 * It covers namespace and variable decls.
 */
TEST_F(TestHelper, SymbolTable3) {
  const char *Expected = "3foo3bar1x";
  PrepareParsingCXX("namespace foo::bar { constexpr int x = 42; }\n");

  RunAction;
  auto &Actual = GetSymbols(DB);
  CompareSymbol(Actual[0], Expected);
}

} /* namespace clang */
