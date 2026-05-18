#include "context.h"
#include "ast.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

std::string DatabaseContext::CurrentScope() const {
  long i;
  for (i = DeclContexts.size() - 1; i >= 0; --i) {
    if (isCLinkage(DeclContexts[i])) {
      break;
    }
  }

  if (i >= 0) {
    /* Special treatment for an extern "C" */
    std::string Ret = "6extern";
    size_t Start = NameSizes[i];
    Ret += WholeName.substr(Start, WholeName.size() - Start);
    return Ret;
  }
  return WholeName;
}

void DatabaseContext::IterateScope(DeclContext *Scope) {
  std::string CurScope = CurrentScope();

  for (auto Iter = Scope->decls_begin(); Iter != Scope->decls_end(); Iter++) {
    if (auto *ND = llvm::dyn_cast<NamespaceDecl>(*Iter)) {
      std::string ShortName = EncodeNs(ND->getName());
      /*
       * Do not use this->InsertIntoNamespace,
       * which also adds `ND` to the DB.
       */
      Database.InsertIntoNamespace(CurScope, ShortName, "9namespace");
      this->VisitNamespaceDecl(ND);
    } else if (auto *LSD = llvm::dyn_cast<LinkageSpecDecl>(*Iter)) {
      VisitLinkageSpecDecl(LSD);
    } else if (auto *RD = llvm::dyn_cast<CXXRecordDecl>(*Iter)) {
      std::string ShortName;
      CXXRecordMember(ShortName, RD);
      InsertIntoNamespace(CurScope, ShortName, TypeofCXXRecordDecl(RD), RD);
      VisitCXXRecordDecl(RD);
    } else if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(*Iter)) {
      std::string ShortName;
      ClassTemplateMember(ShortName, CTD);
      InsertIntoNamespace(CurScope, ShortName,
                          TypeofCXXRecordDecl(CTD->getTemplatedDecl()), CTD);
      VisitClassTemplateDecl(CTD);
    } else if (auto *_ = llvm::dyn_cast<CXXDeductionGuideDecl>(*Iter)) {
      /* ignored for now. */
    } else if (auto *FD = llvm::dyn_cast<FunctionDecl>(*Iter)) {
      std::string ShortName;
      FunctionMember(ShortName, FD);
      InsertIntoNamespace(CurScope, ShortName, TypeofFunctionDecl(FD), FD);
    } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(*Iter)) {
      std::string ShortName;
      FunctionTemplateMember(ShortName, FTD);
      InsertIntoNamespace(CurScope, ShortName, TypeofFunctionTemplateDecl(FTD),
                          FTD);
    } else if (auto *VD = llvm::dyn_cast<VarDecl>(*Iter)) {
      std::string ShortName = EncodeNs(VD->getName());
      InsertIntoNamespace(CurScope, ShortName,
                          TypeofVarDecl(VD, AccessSpecifier::AS_none), VD);
    }
  }
}

void DatabaseContext::IterateClass(CXXRecordDecl *RD) {
  AccessSpecifier Access =
      RD->isClass() ? AccessSpecifier::AS_private : AccessSpecifier::AS_public;
  auto Iter = RD->decls_begin();
  auto End = RD->decls_end();
  if (Iter == End) {
    return;
  }
  /* Must skip the first one -- the struct/class body */
  for (Iter++; Iter != End; Iter++) {
    if (auto *RD = llvm::dyn_cast<CXXRecordDecl>(*Iter)) {
      std::string ShortName;
      CXXRecordMember(ShortName, RD);
      InsertIntoClass(CurrentScope(), ShortName,
                      TypeofLocalCXXRecordDecl(RD, Access), RD);
      VisitCXXRecordDecl(RD);
    } else if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(*Iter)) {
      std::string ShortName;
      ClassTemplateMember(ShortName, CTD);
      InsertIntoClass(CurrentScope(), ShortName,
                      TypeofLocalCXXRecordDecl(CTD->getTemplatedDecl(), Access),
                      CTD);
      VisitClassTemplateDecl(CTD);
    } else if (auto *FD = llvm::dyn_cast<FunctionDecl>(*Iter)) {
      std::string ShortName;
      FunctionMember(ShortName, FD);
      InsertIntoClass(
          CurrentScope(), ShortName,
          TypeofLocalCXXMethodDecl(dyn_cast<CXXMethodDecl>(FD), Access), FD);
    } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(*Iter)) {
      std::string ShortName;
      FunctionTemplateMember(ShortName, FTD);
      InsertIntoClass(
          CurrentScope(), ShortName,
          TypeofLocalCXXMethodDecl(
              dyn_cast<CXXMethodDecl>(FTD->getTemplatedDecl()), Access),
          FTD);
    } else if (auto *VD = llvm::dyn_cast<VarDecl>(*Iter)) {
      std::string ShortName = EncodeNs(VD->getName());
      InsertIntoClass(CurrentScope(), ShortName, TypeofVarDecl(VD, Access), VD);
    } else if (auto *AccessDecl = llvm::dyn_cast<AccessSpecDecl>(*Iter)) {
      Access = AccessDecl->getAccess();
    } else if (auto *FD = llvm::dyn_cast<FieldDecl>(*Iter)) {
      std::string ShortName = EncodeNs(FD->getName());
      InsertIntoClass(CurrentScope(), ShortName, TypeofClassMember(FD, Access),
                      FD);
    }
  }
}

void DatabaseContext::InsertIntoNamespace(std::string CurScope,
                                          std::string &ShortName,
                                          std::string SymType, Decl *CurDecl) {
  int Result = Database.InsertIntoNamespace(CurScope, ShortName, SymType);
  if (unlikely(Result != 0)) {
    llvm::errs() << "Failed to insert into namespace: " << CurScope << "\n";
  }

  CurScope += ShortName;
  std::string &WholeName = CurScope;
  Result = Database.InsertSymbol(WholeName, CurDecl->getLocation());
  if (unlikely(Result != 0)) {
    llvm::errs() << "Failed to insert symbol: " << WholeName << "\n";
  }
}

void DatabaseContext::InsertIntoClass(std::string CurScope,
                                      std::string &ShortName,
                                      std::string SymType, Decl *CurDecl) {
  int Result = Database.InsertIntoClass(CurrentScope(), ShortName, SymType);
  if (unlikely(Result != 0)) {
    llvm::errs() << "Failed to insert into class: " << CurrentScope() << "\n";
  }

  CurScope += ShortName;
  std::string &WholeName = CurScope;
  Result = Database.InsertSymbol(WholeName, CurDecl->getLocation());
  if (unlikely(Result != 0)) {
    llvm::errs() << "Failed to insert symbol: " << WholeName << "\n";
  }
}

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */
