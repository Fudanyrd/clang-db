#include "test.h"

namespace clang {

TEST(SQLite, Transaction) {
  database::SqliteDatabase DB;
  ASSERT_EQ(0, DB.TransactionBegin());
  ASSERT_EQ(0, DB.Commit());
}

TEST(SQLite, Query) {
  database::SqliteDatabase DB;
  ASSERT_TRUE(DB);

  /* namespace foo */
  const char *Namespaces[] = {"", "3foo", "9namespace"};

  ASSERT_EQ(0, DB.TransactionBegin());
  ASSERT_EQ(
      0, DB.InsertIntoNamespace(Namespaces[0], Namespaces[1], Namespaces[2]));
  ASSERT_EQ(0, DB.Commit());

  /* Try retrieve the inserted tuple. */
  std::string Name, Member, Type;

  using IteratorType = database::IteratorBase<std::string, std::string>;
  char Buffer[IteratorType::AllocSize()];
  auto *It = DB.QueryNamespace(Namespaces[0], Buffer);
  ASSERT_TRUE(It);
  ASSERT_EQ(1, It->next(Name, Type));
  EXPECT_EQ(Namespaces[1], Name);
  EXPECT_EQ(Namespaces[2], Type);

  ASSERT_EQ(0, It->next(Name, Type));
  /* Explicitly destroys the iterator. */
  It->~IteratorType();
}

} /* namespace clang */
