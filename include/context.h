#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/PartialDiagnostic.h>
#include <clang/Frontend/FrontendAction.h>
#include <memory>
#include <vector>

#include "common.h"
#include "storage.h"
#include "typing.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

struct BuildVisitor;
struct BuildDatabaseConsumer;
struct ClassDeclVisitor; /* internal use. */

inline std::string TypeofFunctionDecl(FunctionDecl *FD) {
  return MangleType(FD->getReturnType().getTypePtr());
}

inline std::string TypeofFunctionTemplateDecl(FunctionTemplateDecl *FTD) {
  auto *Fn = FTD->getTemplatedDecl();
  return TypeofFunctionDecl(cast<FunctionDecl>(Fn));
}

struct DatabaseContext {
private:
  std::vector<size_t> NameSizes;
  std::string WholeName;
  std::vector<DeclContext *> DeclContexts;
  DatabaseInterface &Database;

  static bool isCLinkage(DeclContext *Ctx) {
    auto *LSD = llvm::dyn_cast<LinkageSpecDecl>(Ctx);
    return LSD && LSD->getLanguage() == LinkageSpecLanguageIDs::C;
  }

  void LeaveDeclContext() {
    size_t Level = DeclContexts.size();
    assert(Level == NameSizes.size());
    assert(Level > 1);
    NameSizes.pop_back();
    DeclContexts.pop_back();
    WholeName.resize(NameSizes[Level - 2]);
  }

  void EnterDeclContext(DeclContext *Ctx, llvm::StringRef Name) {
    EnterDeclContext(Ctx, Name.str());
  }
  void EnterDeclContext(DeclContext *Ctx, const std::string &Name) {
    WholeName += Name;
    NameSizes.push_back(WholeName.size());
    DeclContexts.push_back(Ctx);
  }
  void EnterDeclContext(DeclContext *Ctx, const char *Name) {
    WholeName += Name;
    NameSizes.push_back(WholeName.size());
    DeclContexts.push_back(Ctx);
  }

  void Reset() {
    WholeName.clear();
    NameSizes.clear();
    DeclContexts.clear();
  }

  /**
   * Iterate over the members of a namespace, tranlation
   * unit, or linkage spec.
   */
  void IterateScope(DeclContext *Scope);

  void IterateClass(CXXRecordDecl *RD);

  /**
   * `XXXMember` methods are used to encode the `member` column
   * of the declarations.
   *
   * Default is: EncodeNs(XXX->getName()).
   */

  void FunctionMember(std::string &Dest, FunctionDecl *FD) {
    Dest += EncodeFunctionName(FD);
    const auto *Proto =
        dyn_cast<const FunctionProtoType>(FD->getType().getTypePtr());
    assert(Proto != nullptr);
    MangleFunctionParmList(Dest, Proto);
  }

  void FunctionTemplateMember(std::string &Dest, FunctionTemplateDecl *FTD) {
    Dest += MangleFunctionTemplate(*FTD);
    auto *Fn = FTD->getTemplatedDecl();
    const auto *Proto =
        dyn_cast<const FunctionProtoType>(Fn->getType().getTypePtr());
    assert(Proto != nullptr);
    MangleFunctionParmList(Dest, Proto);
  }

  void CXXMethodMember(std::string &Dest, CXXMethodDecl *MD) {
    FunctionMember(Dest, MD);
  }

  void ClassTemplateMember(std::string &Dest, ClassTemplateDecl *TD) {
    Dest += MangleClassTemplate(*TD);
  }

  void InsertIntoNamespace(std::string CurScope, std::string &ShortName,
                           std::string SymType, Decl *CurDecl);

  void InsertIntoClass(std::string CurScope, std::string &ShortName,
                       std::string SymType, Decl *CurDecl);

public:
  DatabaseContext(DatabaseInterface &DB) : Database(DB) {
    DB.TransactionBegin();
  }
  ~DatabaseContext() { Database.Commit(); }

  void VisitTranslationUnitDecl(TranslationUnitDecl *TU) {
    Reset();
    EnterDeclContext(TU, "");
    IterateScope(TU);
    Reset();
  }
  void VisitNamespaceDecl(NamespaceDecl *NS) {
    EnterDeclContext(NS, EncodeNs(NS->getName()));
    IterateScope(NS);
    LeaveDeclContext();
  }
  void VisitLinkageSpecDecl(LinkageSpecDecl *LSD) {
    EnterDeclContext(LSD, "");
    IterateScope(LSD);
    LeaveDeclContext();
  }

  void VisitCXXRecordDecl(CXXRecordDecl *RD) {
    EnterDeclContext(RD, EncodeNs(RD->getName()));
    IterateClass(RD);
    LeaveDeclContext();
  }

  void VisitClassTemplateDecl(ClassTemplateDecl *TD) {
    std::string ShortName;
    ClassTemplateMember(ShortName, TD);
    EnterDeclContext(TD->getTemplatedDecl(), ShortName);
    IterateClass(TD->getTemplatedDecl());
    LeaveDeclContext();
  }

  std::string CurrentScope() const;
};

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */
