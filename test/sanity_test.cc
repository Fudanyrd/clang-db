#include <gtest/gtest.h>

static inline int Add(int lhs, int rhs) { return lhs + rhs; }

/**
 * Check that gtest is sane.
 */
TEST(Gtest, Sanity) { ASSERT_EQ(3, Add(1, 2)); }

TEST(Gtest, DISABLED_Disabling) { ASSERT_EQ(3, Add(1, 2)); }
