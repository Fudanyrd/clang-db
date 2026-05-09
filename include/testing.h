#ifndef __TESTING_H__
#define __TESTING_H__ (1)

/**
 * Only enable this header file while testing.
 */
#if defined GOOGLETEST_INCLUDE_GTEST_GTEST_H_

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/Support/MemoryBuffer.h>

namespace clang {

class TestHelper : public ::testing::Test {
public:
  CompilerInstance Instance;
  std::shared_ptr<CompilerInvocation> Invocation;
  static constexpr char FILE[] = "test.cc";
  static constexpr char TRIPLE[] = "x86_64-unknown-linux-gnu";

  TestHelper()
      : Invocation(std::make_shared<CompilerInvocation>()), Instance() {
    Invocation = Invocation;
    Invocation->getFrontendOpts().ProgramAction = frontend::PluginAction;
    Invocation->getTargetOpts().Triple = "x86_64-unknown-linux-gnu";

    Instance.setInvocation(Invocation);
    Instance.createDiagnostics();
  }

  ~TestHelper() = default;
};

} /* namespace clang */

#endif /* GOOGLETEST_INCLUDE_GTEST_GTEST_H_ */

#endif /* __TESTING_H__ */
