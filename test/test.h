#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__ (1)

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
} /* namespace clang */

#endif /* __TEST_COMMON_H__ */
