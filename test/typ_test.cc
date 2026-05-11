#include <gtest/gtest.h>

#include <clangdb.h>

namespace clang {

TEST_F(TestHelper, BuiltinTypes) {
  static const char *code = "namespace foo { const int x; }";
  Invocation->getPreprocessorOpts().addRemappedFile(
      FILE, llvm::MemoryBuffer::getMemBuffer(code).release());
  Invocation->getPreprocessorOpts().addRemappedFile(
      FILE, llvm::MemoryBuffer::getMemBuffer(code).release());

  database::InMemoryDatabase DB;
  std::unique_ptr<FrontendAction> action =
      std::make_unique<database::BuildDatabaseAction>(DB);
  ASSERT_TRUE(action);

  Instance.ExecuteAction(*action);
}

} // namespace clang
