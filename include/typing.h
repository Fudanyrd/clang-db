#ifndef __TYPING_H__
#define __TYPING_H__ (1)

#include <map>
#include <string>

#include <clang/AST/Type.h>
#include <clang/AST/TypeVisitor.h>

#include "common.h"

namespace clang {

namespace database _CLANGDB_VISIBILITY {

std::string MangleType(const Type *TypePtr);

std::string MangleRecordDecl(const TagDecl *RD);
std::string MangleNestedNameSpecifier(const NestedNameSpecifier *NNS);

std::string MangleTemplateParameterList(const TemplateParameterList &List);

std::string MangleNamespaceStack(const std::vector<DeclContext *> &Nesting,
                                 size_t StartIdx = 0);

inline std::string MangleClassTemplate(ClassTemplateDecl &CTD) {
  return EncodeNs(CTD.getTemplatedDecl()->getName()) +
         MangleTemplateParameterList(*CTD.getTemplateParameters());
}

inline std::string MangleFunctionTemplate(FunctionTemplateDecl &FTD) {
  FunctionDecl *Fn = FTD.getTemplatedDecl();
  return EncodeNs(Fn->getName()) +
         MangleTemplateParameterList(*FTD.getTemplateParameters()) +
         MangleType(Fn->getReturnType().getTypePtr());
}

inline std::string MangleTemplateName(const TemplateName &TN) {
  TemplateDecl *TD = TN.getAsTemplateDecl();
  if (auto *CTD = llvm::dyn_cast<ClassTemplateDecl>(TD)) {
    return MangleClassTemplate(*CTD);
  } else if (auto *FTD = llvm::dyn_cast<FunctionTemplateDecl>(TD)) {
    return MangleFunctionTemplate(*FTD);
  } else {
    llvm::errs() << "Unknown template declaration kind: " << TD->getKind()
                 << "\n";
    abort();
  }
}

struct ManglingTypeVisitor
    : public clang::TypeVisitor<ManglingTypeVisitor, std::string> {
  std::map<std::string, std::string> &Typedefs;

  ManglingTypeVisitor(std::map<std::string, std::string> &Typedefs)
      : Typedefs(Typedefs) {}

  std::string VisitBuiltinType(const BuiltinType *BT);

  std::string VisitPointerType(const PointerType *PT) {
    return "P" + Visit(PT->getPointeeType().getTypePtr());
  }

  std::string VisitReferenceType(const ReferenceType *RT) {
    return "R" + Visit(RT->getPointeeType().getTypePtr());
  }
};

} // namespace database _CLANGDB_VISIBILITY

} /* namespace clang */

#endif /* __TYPING_H__ */
