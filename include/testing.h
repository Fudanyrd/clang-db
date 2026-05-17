#ifndef __TESTING_H__
#define __TESTING_H__ (1)

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/Support/MemoryBuffer.h>

#include "storage.h"

/**
 * Only enable this header file while testing.
 */
#if defined GOOGLETEST_INCLUDE_GTEST_GTEST_H_

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
    Invocation->getTargetOpts().Triple = TRIPLE;

    Instance.setInvocation(Invocation);
    Instance.createDiagnostics();
  }

  void PrepareParsingCXX(const char *CXXCode) {
    Invocation->getPreprocessorOpts().addRemappedFile(
        FILE, llvm::MemoryBuffer::getMemBuffer(CXXCode).release());
    Invocation->getFrontendOpts().Inputs.push_back(
        clang::FrontendInputFile(FILE, clang::Language::CXX));
    Invocation->getFrontendOpts().ProgramAction =
        clang::frontend::ParseSyntaxOnly;
    Invocation->getTargetOpts().Triple = TRIPLE;
    Invocation->getLangOpts().CPlusPlus = true;
    Invocation->getLangOpts().CPlusPlus11 = true;
  }

  std::vector<std::tuple<std::string, std::string, std::string>> &
  GetNamespaces(database::InMemoryDatabase &DB) {
    return DB.Namespaces;
  }

  std::vector<std::tuple<std::string, std::string, std::string>> &
  GetClasses(database::InMemoryDatabase &DB) {
    return DB.Classes;
  }

  std::vector<std::tuple<std::string, std::string, int>> &
  GetSymbols(database::InMemoryDatabase &DB) {
    return DB.Symbols;
  }

  ~TestHelper() = default;
};

} /* namespace clang */

#endif /* GOOGLETEST_INCLUDE_GTEST_GTEST_H_ */

#endif /* __TESTING_H__ */
