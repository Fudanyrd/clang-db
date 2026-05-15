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
      Database.InsertIntoNamespace(CurScope, ShortName, "9namespace");
      this->VisitNamespaceDecl(ND);
    } else if (auto *LSD = llvm::dyn_cast<LinkageSpecDecl>(*Iter)) {
      VisitLinkageSpecDecl(LSD);
    } else if (auto *RD = llvm::dyn_cast<CXXRecordDecl>(*Iter)) {
      std::string ShortName = EncodeNs(RD->getName());
      Database.InsertIntoNamespace(CurScope, ShortName,
                                   TypeofCXXRecordDecl(RD));
      VisitCXXRecordDecl(RD);
    } else if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(*Iter)) {
      std::string ShortName;
      ClassTemplateMember(ShortName, CTD);
      Database.InsertIntoNamespace(
          CurScope, ShortName, TypeofCXXRecordDecl(CTD->getTemplatedDecl()));
      VisitClassTemplateDecl(CTD);
    } else if (auto *FD = llvm::dyn_cast<FunctionDecl>(*Iter)) {
      std::string ShortName;
      FunctionMember(ShortName, FD);
      Database.InsertIntoNamespace(CurScope, ShortName, TypeofFunctionDecl(FD));
    } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(*Iter)) {
      std::string ShortName;
      FunctionTemplateMember(ShortName, FTD);
      Database.InsertIntoNamespace(CurScope, ShortName,
                                   TypeofFunctionTemplateDecl(FTD));
    } else if (auto *VD = llvm::dyn_cast<VarDecl>(*Iter)) {
      std::string ShortName = EncodeNs(VD->getName());
      Database.InsertIntoNamespace(CurScope, ShortName,
                                   TypeofVarDecl(VD, AccessSpecifier::AS_none));
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
      std::string ShortName = EncodeNs(RD->getName());
      Database.InsertIntoClass(CurrentScope(), ShortName,
                               TypeofLocalCXXRecordDecl(RD, Access));
      VisitCXXRecordDecl(RD);
    } else if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(*Iter)) {
      std::string ShortName;
      ClassTemplateMember(ShortName, CTD);
      Database.InsertIntoClass(
          CurrentScope(), ShortName,
          TypeofLocalCXXRecordDecl(CTD->getTemplatedDecl(), Access));
      VisitClassTemplateDecl(CTD);
    } else if (auto *FD = llvm::dyn_cast<FunctionDecl>(*Iter)) {
      std::string ShortName;
      FunctionMember(ShortName, FD);
      Database.InsertIntoClass(
          CurrentScope(), ShortName,
          TypeofLocalCXXMethodDecl(dyn_cast<CXXMethodDecl>(FD), Access));
    } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(*Iter)) {
      std::string ShortName;
      FunctionTemplateMember(ShortName, FTD);
      Database.InsertIntoClass(
          CurrentScope(), ShortName,
          TypeofLocalCXXMethodDecl(
              dyn_cast<CXXMethodDecl>(FTD->getTemplatedDecl()), Access));
    } else if (auto *VD = llvm::dyn_cast<VarDecl>(*Iter)) {
      std::string ShortName = EncodeNs(VD->getName());
      Database.InsertIntoClass(CurrentScope(), ShortName,
                               TypeofVarDecl(VD, Access));
    } else if (auto *AccessDecl = llvm::dyn_cast<AccessSpecDecl>(*Iter)) {
      Access = AccessDecl->getAccess();
    } else if (auto *FD = llvm::dyn_cast<FieldDecl>(*Iter)) {
      std::string ShortName = EncodeNs(FD->getName());
      Database.InsertIntoClass(CurrentScope(), ShortName,
                               TypeofClassMember(FD, Access));
    }
  }
}

} /* namespace database _CLANGDB_VISIBILITY */

} /* namespace clang */
