#ifndef __AST_H__
#define __AST_H__ (1)

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/PartialDiagnostic.h>
#include <clang/Frontend/FrontendAction.h>
#include <memory>
#include <stack>

#include "storage.h"
#include "typing.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

struct BuildVisitor;
struct BuildDatabaseConsumer;
struct ClassDeclVisitor; /* internal use. */

/**
 * Context for {@link BuildVisitor} and {@link BuildDatabaseConsumer}.
 */
struct BuildVisitorContext {
public:
  /**
   * Prefix for anonymous namespaces. The whole name of an anonymous namespace
   * will be this prefix followed by a unique number, e.g., "_GLOBAL__N_0"
   */
  static constexpr char AnonymousNamespacePrefix[] = "_GLOBAL__N_";

private:
  friend struct BuildVisitor;
  friend struct BuildDatabaseConsumer;
  friend struct ClassDeclVisitor; /* internal use. */
  DatabaseInterface *DB;

  /**
   * For both `NamespaceDecl`, `TranslationUnitDecl`, and `CXXRecordDecl`.
   */
  std::vector<DeclContext *> NamespaceStack;

  /**
   * Generating unique names for anonymous namespaces.
   */
  size_t AnonymousNamespaceCounter;
  ASTContext *CompilerContext;

  BuildVisitorContext(DatabaseInterface *Database, ASTContext *Ctx)
      : DB(Database), NamespaceStack(), AnonymousNamespaceCounter(0),
        CompilerContext(Ctx) {}

  void EnterNamespaceOrClass(DeclContext *DeclCtx, llvm::StringRef Name) {
    auto *TopLevel = NamespaceStack.back();
    assert(TopLevel == DeclCtx->getParent() &&
           "NamespaceDecl's parent should be the current decl context");

    auto CurNsWhole = CurrentNamespace();
    auto ShortNs = EncodeNs(Name);
    NamespaceStack.push_back(DeclCtx);
    auto NewNsWhole = CurNsWhole + ShortNs;
    if (dyn_cast<CXXRecordDecl>(TopLevel)) {
      DB->InsertIntoClass(CurNsWhole, ShortNs, NewNsWhole);
    } else {
      DB->InsertIntoNamespace(CurNsWhole, ShortNs, NewNsWhole);
    }
  }

  void EnterNamespace(NamespaceDecl *ND) {
    EnterNamespaceOrClass(ND, ND->getName());
  }

  void EnterClass(CXXRecordDecl *RD) {
    EnterNamespaceOrClass(RD, RD->getName());
  }

  void EnterTranslationUnit(TranslationUnitDecl *TU) {
    NamespaceStack.clear(); /* clear whatever exists in the stack. */
    NamespaceStack.push_back(TU);
  }

  void LeaveNamespaceOrClass() { NamespaceStack.pop_back(); }

  DeclContext *CurrentDeclContext() const {
    if (NamespaceStack.empty()) {
      llvm::errs()
          << "Must have at least one TransaltionUnitDecl in the stack\n";
      assert(0);
    }
    return NamespaceStack.back();
  }

  std::string GetAnonymousNamespaceName() {
    return AnonymousNamespacePrefix +
           std::to_string(AnonymousNamespaceCounter++);
  }

  void PopUntilFindParent(DeclContext *Parent) {
    while (!NamespaceStack.empty() && NamespaceStack.back() != Parent) {
      NamespaceStack.pop_back();
    }
  }

public:
  /**
   * Returns mangled whole name of current namespace, e.g.,
   * "3foo3bar" for "namespace foo { namespace bar { } }",
   * but "" for the global namespace.
   */
  std::string CurrentNamespace() const;

  DatabaseInterface &GetDatabase() const {
    clangdb_check_internal(
        DB != nullptr && "DatabaseInterface is not set in BuildVisitorContext");
    return *DB;
  }
};

/**
 * The visitor for building the database. It traverses the AST and
 * fills the database tables, as specified in ReadMe.
 */
struct BuildVisitor : public RecursiveASTVisitor<BuildVisitor> {
  BuildVisitor(BuildVisitorContext &Ctx) : Context(Ctx) {}

  bool TraverseNamespaceDecl(NamespaceDecl *ND);

  bool TraverseTranslationUnitDecl(TranslationUnitDecl *TU) {
    Context.EnterTranslationUnit(TU);
    return RecursiveASTVisitor<BuildVisitor>::TraverseTranslationUnitDecl(TU);
  }

  bool TraverseCXXRecordDecl(CXXRecordDecl *RD);
  bool TraverseClassTemplateDecl(ClassTemplateDecl *CTD);

private:
  std::map<std::string, std::string> Typedefs;
  BuildVisitorContext &Context;
};

struct BuildDatabaseConsumer : public ASTConsumer {
  BuildVisitorContext Context;
  BuildDatabaseConsumer(DatabaseInterface *Database)
      : Context(Database, nullptr) {}

  void HandleTranslationUnit(ASTContext &Ctx) override {
    BuildVisitor Visitor(this->Context);
    Context.CompilerContext = &Ctx;
    Visitor.TraverseDecl(Ctx.getTranslationUnitDecl());
  }
};

struct BuildDatabaseAction : public PluginASTAction {
  DatabaseInterface *DB;

  BuildDatabaseAction(void) : DB(nullptr) {}
  BuildDatabaseAction(DatabaseInterface &Database) : DB(&Database) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef InFile) override {
    return std::make_unique<BuildDatabaseConsumer>(this->DB);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
};

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */

#endif /* __AST_H__ */
