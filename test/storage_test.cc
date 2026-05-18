#include "test.h"

namespace clang {

TEST(SQLite, Transaction) {
  database::SqliteDatabase DB;
  ASSERT_EQ(0, DB.TransactionBegin());
  ASSERT_EQ(0, DB.Commit());
}

} /* namespace clang */
