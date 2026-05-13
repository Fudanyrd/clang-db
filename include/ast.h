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
 * <h3>ReadMe - Type Column Spec</h3>
 *
 * The "type" column for a C++ record is "5class" or "6struct".
 */
inline const char *TypeofCXXRecordDecl(CXXRecordDecl *RD) {
  const char *Type;
  if (RD->isStruct()) {
    Type = "6struct";
  } else if (RD->isClass()) {
    Type = "5class";
  } else if (RD->isUnion()) {
    Type = "5union";
  } else if (RD->isEnum()) {
    Type = "4enum";
  } else {
    llvm_unreachable("Unknown CXXRecordDecl kind");
  }
  return Type;
}

inline const char *EncodeAccessSpecifier(AccessSpecifier Access) {
  switch (Access) {
  case AS_public:
    return "6public";
  case AS_protected:
    return "9protected";
  case AS_private:
    return "7private";
  }
  /* AS_none: */
  llvm_unreachable("Unknown AccessSpecifier");
}

inline std::string TypeofLocalCXXRecordDecl(CXXRecordDecl *RD,
                                            AccessSpecifier Access) {
  const char *Type = TypeofCXXRecordDecl(RD);
  std::string Ret = Type;
  Ret += EncodeAccessSpecifier(Access);
  return Ret;
}

std::string TypeofLocalCXXMethodDecl(CXXMethodDecl *Method,
                                     AccessSpecifier Access);

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

  void EnterNamespaceOrClass(DeclContext *DeclCtx, llvm::StringRef Name,
                             std::string Type) {
    auto *TopLevel = NamespaceStack.back();
    assert(TopLevel == DeclCtx->getParent() &&
           "NamespaceDecl's parent should be the current decl context");

    auto CurNsWhole = CurrentNamespace();
    auto ShortNs = EncodeNs(Name);
    NamespaceStack.push_back(DeclCtx);
    if (dyn_cast<CXXRecordDecl>(TopLevel)) {
      DB->InsertIntoClass(CurNsWhole, ShortNs, Type);
    } else {
      DB->InsertIntoNamespace(CurNsWhole, ShortNs, Type);
    }
  }

  void EnterNamespace(NamespaceDecl *ND) {
    EnterNamespaceOrClass(ND, ND->getName(), "9namespace");
  }

  void EnterLinkageSpecDecl(LinkageSpecDecl *LSD) {
    NamespaceStack.push_back(LSD);
  }

  bool TopIsCXXRecordDecl() const {
    assert(!NamespaceStack.empty() && "NamespaceStack should not be empty");
    return isa<CXXRecordDecl>(NamespaceStack.back());
  }
  bool TopIsCLinkage() const {
    LinkageSpecDecl *LSD = dyn_cast<LinkageSpecDecl>(NamespaceStack.back());
    return LSD != nullptr && LSD->getLanguage() == LinkageSpecLanguageIDs::C;
  }

  void EnterClass(CXXRecordDecl *RD) {
    const char *Type = TypeofCXXRecordDecl(RD);
    EnterNamespaceOrClass(RD, RD->getName(), Type);
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
    DeclContext *TranslationUnit = NamespaceStack[0];
    while (!NamespaceStack.empty() && NamespaceStack.back() != Parent) {
      NamespaceStack.pop_back();
    }
    if (NamespaceStack.empty()) {
      /* Add a break point here to see what's happening. */
      NamespaceStack.push_back(TranslationUnit);
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

  bool TraverseFunctionDecl(FunctionDecl *FD);
  bool TraverseFunctionTemplateDecl(FunctionTemplateDecl *FTD);

  bool TraverseLinkageSpecDecl(LinkageSpecDecl *LSD) {
    Context.EnterLinkageSpecDecl(LSD);
    return RecursiveASTVisitor<BuildVisitor>::TraverseLinkageSpecDecl(LSD);
  }

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
    DB = new InMemoryDatabase();
    return true;
  }
};

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */

#endif /* __AST_H__ */
