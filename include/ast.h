#ifndef __AST_H__
#define __AST_H__ (1)

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Frontend/FrontendAction.h>
#include <memory>

#include "typing.h"

namespace clang {

namespace database {

struct BuildVisitor : public RecursiveASTVisitor<BuildVisitor> {
  BuildVisitor() {}

  bool VisitType(Type *T) {
    llvm::errs() << mangleType(T, Typedefs) << "\n";
    return true;
  }

private:
  std::map<std::string, std::string> Typedefs;
};

struct BuildDatabaseConsumer : public ASTConsumer {
  BuildDatabaseConsumer() {}

  void HandleTranslationUnit(ASTContext &Ctx) override {
    BuildVisitor Visitor;
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
  }
};

struct BuildDatabaseAction : public PluginASTAction {
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return std::make_unique<BuildDatabaseConsumer>();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} /* namespace database */

} /* namespace clang */

#endif /* __AST_H__ */
