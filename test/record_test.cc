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
    EXPECT_EQ(TupleVec.size(), ExpectedSize);                                  \
    for (size_t I = 0; I < ExpectedSize; I++) {                                \
      EXPECT_EQ(std::get<0>(TupleVec[I]), Expected[I][0]);                     \
      EXPECT_EQ(std::get<1>(TupleVec[I]), Expected[I][1]);                     \
      EXPECT_EQ(std::get<2>(TupleVec[I]), Expected[I][2]);                     \
    }                                                                          \
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

} /* namespace clang */
