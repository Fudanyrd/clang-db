#include "test.h"

DefNormalizePath;

TEST(Util, NormalizePath) {
#define Test(Case, Expected)                                                   \
  do {                                                                         \
    std::vector<char> Normalized = NormalizePathName(Case);                    \
    EXPECT_EQ(0, strcmp(Expected, Normalized.data()));                         \
  } while (0)

  Test("", "");
  Test("/a.cc", "/a.cc");
  Test("./foo/bar", "foo/bar");

  Test("/a/b/../c/./d", "/a/c/d");
  Test("a/b/../../c", "c");
  Test("a/./b/./c", "a/b/c");
}
